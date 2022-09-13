#include "hsk_raytracingstage.hpp"
#include "../base/hsk_shadercompiler.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/globalcomponents/hsk_cameramanager.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/globalcomponents/hsk_materialbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_texturestore.hpp"
#include "../scenegraph/globalcomponents/hsk_tlasmanager.hpp"
#include "../utility/hsk_pipelinebuilder.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "../utility/hsk_shaderstagecreateinfos.hpp"
#include <array>

namespace hsk {
    RaytracingStageShaderconfig RaytracingStageShaderconfig::Basic()
    {
        return RaytracingStageShaderconfig{.RaygenShaderpath     = "../hsk_rt_rpf/src/shaders/rt_basic/raygen.rgen",
                                           .MissShaderpath       = "../hsk_rt_rpf/src/shaders/rt_basic/miss.rmiss",
                                           .ClosesthitShaderpath = "../hsk_rt_rpf/src/shaders/rt_basic/closesthit.rchit"};
    }
    void RaytracingStage::Init(const VkContext* context, Scene* scene, ManagedImage* environmentMap, ManagedImage* noiseSource, const RaytracingStageShaderconfig& shaderconfig)
    {
        mContext        = context;
        mScene          = scene;
        mEnvironmentMap = environmentMap;
        mNoiseSource    = noiseSource;

        mRaygenShader.Path     = shaderconfig.RaygenShaderpath;
        mMissShader.Path       = shaderconfig.MissShaderpath;
        mClosesthitShader.Path = shaderconfig.ClosesthitShaderpath;
        mAnyhitShader.Path     = shaderconfig.AnyhitShaderpath;

        VkPhysicalDeviceProperties2 prop2{};
        prop2.sType                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        prop2.pNext                         = &mRayTracingPipelineProperties;
        mRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        vkGetPhysicalDeviceProperties2(mContext->PhysicalDevice, &prop2);

        CreateResolutionDependentComponents();
        CreateFixedSizeComponents();

        // Clear values for all attachments written in the fragment shader
        mClearValues.resize(mColorAttachments.size() + 1);
        for(size_t i = 0; i < mColorAttachments.size(); i++)
        {
            mClearValues[i].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        }
        mClearValues[mColorAttachments.size()].depthStencil = {1.0f, 0};
    }

    void RaytracingStage::OnResized(const VkExtent2D& extent)
    {
        RenderStage::OnResized(extent);
        UpdateDescriptors();
    }

    void RaytracingStage::CreateFixedSizeComponents()
    {
        SetupEnvironmentMap();
        SetupNoiseSource();
        SetupDescriptors();
        CreatePipelineLayout();
        CreateRaytraycingPipeline();
        CreateShaderBindingTables();
    }

    void RaytracingStage::DestroyFixedComponents()
    {
        if(!!mContext)
        {
            VkDevice device = mContext->Device;
            if(!!mPipeline)
            {
                vkDestroyPipeline(device, mPipeline, nullptr);
                mPipeline = nullptr;
            }
            if(!!mPipelineLayout)
            {
                vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
                mPipelineLayout = nullptr;
            }
        }
        std::array<ShaderResource*, 4> shaders({&mRaygenShader, &mMissShader, &mClosesthitShader, &mAnyhitShader});
        for(auto shader : shaders)
        {
            shader->BindingTable.Destroy();
            shader->Module.Destroy();
        }
        mDescriptorSet.Destroy();
        if(!!mEnvironmentMapSampler)
        {
            vkDestroySampler(mContext->Device, mEnvironmentMapSampler, nullptr);
            mEnvironmentMapSampler = nullptr;
        }
        if(!!mNoiseSourceSampler)
        {
            vkDestroySampler(mContext->Device, mNoiseSourceSampler, nullptr);
            mNoiseSourceSampler = nullptr;
        }
    }

    void RaytracingStage::CreateResolutionDependentComponents()
    {
        PrepareAttachments();
    }

    void RaytracingStage::DestroyResolutionDependentComponents()
    {
        for(auto& colorAttachment : mColorAttachments)
        {
            colorAttachment->Destroy();
        }
        mRaytracingRenderTarget.Destroy();
    }

    void RaytracingStage::PrepareAttachments()
    {
        static const VkFormat          colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->Swapchain.extent.width, mContext->Swapchain.extent.height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageLayout            intialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;


        mRaytracingRenderTarget.Create(mContext, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocationCreateFlags, extent, imageUsageFlags, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_ASPECT_COLOR_BIT, RaytracingRenderTargetName);
        mRaytracingRenderTarget.TransitionLayout(VK_IMAGE_LAYOUT_GENERAL);

        mColorAttachments = {&mRaytracingRenderTarget};
    }

    void RaytracingStage::SetupDescriptors()
    {
        VkShaderStageFlags rtShaderStages = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                                            | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        mDescriptorSet.SetDescriptorInfoAt(0, GetAccelerationStructureDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(1, GetRenderTargetDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(2, mScene->GetComponent<CameraManager>()->MakeUboDescriptorInfos(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(3, mScene->GetComponent<GeometryStore>()->GetVertexBufferDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(4, mScene->GetComponent<GeometryStore>()->GetIndexBufferDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(5, mScene->GetComponent<MaterialBuffer>()->GetDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(6, mScene->GetComponent<TextureStore>()->GetDescriptorInfo(rtShaderStages));
        mDescriptorSet.SetDescriptorInfoAt(7, mScene->GetComponent<TlasManager>()->GetTlas().GetMetaBuffer().GetDescriptorInfo(rtShaderStages));
        if(!!mEnvironmentMap)
        {
            mDescriptorSet.SetDescriptorInfoAt(9, GetEnvironmentMapDescriptorInfo());
        }
        if(!!mNoiseSource)
        {
            mDescriptorSet.SetDescriptorInfoAt(10, GetNoiseSourceDescriptorInfo());
        }

        mDescriptorSet.Create(mContext, "RaytraycingPipelineDescriptorSet");
    }

    void RaytracingStage::UpdateDescriptors()
    {
        mDescriptorSet.SetDescriptorInfoAt(1, GetRenderTargetDescriptorInfo(true));

        mDescriptorSet.Update(mContext);
    }

    void RaytracingStage::CreatePipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &mDescriptorSet.GetDescriptorSetLayout();
        VkPushConstantRange pushC{
            .stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .offset     = 0,
            .size       = sizeof(mPushConstant),
        };
        pipelineLayoutCreateInfo.pPushConstantRanges    = &pushC;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        AssertVkResult(vkCreatePipelineLayout(mContext->Device, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));
    }

    void RaytracingStage::RecordFrame(FrameRenderInfo& renderInfo)
    {

        VkCommandBuffer commandBuffer = renderInfo.GetCommandBuffer();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipeline);

        const auto& descriptorsets = mDescriptorSet.GetDescriptorSets();
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0, 1,
                                &(descriptorsets[(renderInfo.GetFrameNumber()) % descriptorsets.size()]), 0, nullptr);
        const uint32_t handle_size_aligned = aligned_size(mRayTracingPipelineProperties.shaderGroupHandleSize, mRayTracingPipelineProperties.shaderGroupHandleAlignment);

        mPushConstant.RngSeed = renderInfo.GetFrameNumber();
        vkCmdPushConstants(commandBuffer, mPipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0,
                           sizeof(mPushConstant), &mPushConstant);

        VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
        raygen_shader_sbt_entry.deviceAddress = mRaygenShader.BindingTable.GetDeviceAddress();
        raygen_shader_sbt_entry.stride        = handle_size_aligned;
        raygen_shader_sbt_entry.size          = handle_size_aligned;

        VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
        miss_shader_sbt_entry.deviceAddress = mMissShader.BindingTable.GetDeviceAddress();
        miss_shader_sbt_entry.stride        = handle_size_aligned;
        miss_shader_sbt_entry.size          = handle_size_aligned;

        VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
        hit_shader_sbt_entry.deviceAddress = mClosesthitShader.BindingTable.GetDeviceAddress();
        hit_shader_sbt_entry.stride        = handle_size_aligned;
        hit_shader_sbt_entry.size          = handle_size_aligned;

        VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->Swapchain.extent.width, mContext->Swapchain.extent.height}};
        mContext->DispatchTable.cmdTraceRaysKHR(commandBuffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry,
                                                scissor.extent.width, scissor.extent.height, 1);
    }

    void RaytracingStage::OnShadersRecompiled(ShaderCompiler* shaderCompiler)
    {
        bool                           rebuildNeeded = false;
        std::array<ShaderResource*, 4> shaders({&mRaygenShader, &mMissShader, &mClosesthitShader, &mAnyhitShader});
        for(auto shader : shaders)
        {
            rebuildNeeded |= shaderCompiler->HasShaderBeenRecompiled(shader->Path);
        }
        if(!rebuildNeeded)
            return;

        VkDevice device = mContext->Device;
        if(!!mPipeline)
        {
            vkDestroyPipeline(device, mPipeline, nullptr);
            mPipeline = nullptr;
        }
        if(!!mPipelineLayout)
        {
            vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
        for(auto shader : shaders)
        {
            shader->BindingTable.Destroy();
            shader->Module.Destroy();
        }
        CreatePipelineLayout();
        CreateRaytraycingPipeline();
        CreateShaderBindingTables();
    }

    void RaytracingStage::CreateShaderBindingTables()
    {
        if(mShaderGroups.size() == 0)
        {
            throw Exception("Create shader groups (usually during rt pipeline creation) first!");
        }
        const uint32_t           handleSize          = mRayTracingPipelineProperties.shaderGroupHandleSize;
        const uint32_t           handleSizeAligned   = aligned_size(mRayTracingPipelineProperties.shaderGroupHandleSize, mRayTracingPipelineProperties.shaderGroupHandleAlignment);
        const uint32_t           handleAlignment     = mRayTracingPipelineProperties.shaderGroupHandleAlignment;
        const uint32_t           groupCount          = static_cast<uint32_t>(mShaderGroups.size());
        const uint32_t           sbtSize             = groupCount * handleSizeAligned;
        const VkBufferUsageFlags sbtBufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VmaMemoryUsage     sbtMemoryFlags      = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        const VmaAllocationCreateFlags sbt_allocation_create_flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        // Raygen
        // Create binding table buffers for each shader type

        if(mRaygenShader.Path.size() > 0)
        {
            mRaygenShader.BindingTable.Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags, "RaygenShaderBindingTable");
        }
        if(mMissShader.Path.size() > 0)
        {
            mMissShader.BindingTable.Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags, "MissShaderBindingTable");
        }
        if(mClosesthitShader.Path.size() > 0)
        {
            mClosesthitShader.BindingTable.Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags, "ClosesthitShaderBindingTable");
        }
        if(mAnyhitShader.Path.size() > 0)
        {
            mAnyhitShader.BindingTable.Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags, "AnyhitShaderBindingTable");
        }

        // Copy the pipeline's shader handles into a host buffer
        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        AssertVkResult(mContext->DispatchTable.getRayTracingShaderGroupHandlesKHR(mPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

        uint32_t offset = 0;

        // Copy the shader handles from the host buffer to the binding tables
        if(mRaygenShader.Path.size() > 0)
        {
            mRaygenShader.BindingTable.MapAndWrite(shaderHandleStorage.data() + handleSizeAligned * offset, handleSize);
            offset++;
        }
        if(mMissShader.Path.size() > 0)
        {
            mMissShader.BindingTable.MapAndWrite(shaderHandleStorage.data() + handleSizeAligned * offset, handleSize);
            offset++;
        }
        if(mClosesthitShader.Path.size() > 0)
        {
            mClosesthitShader.BindingTable.MapAndWrite(shaderHandleStorage.data() + handleSizeAligned * offset, handleSize);
            offset++;
        }
        if(mAnyhitShader.Path.size() > 0)
        {
            mAnyhitShader.BindingTable.MapAndWrite(shaderHandleStorage.data() + handleSizeAligned * offset, handleSize);
            offset++;
        }
    }

    void RaytracingStage::CreateRaytraycingPipeline()
    {
        // Setup ray tracing shader groups
        // Each shader group points at the corresponding shader in the pipeline

        // shader stages
        ShaderStageCreateInfos shaderStageCreateInfos;

        // Ray generation group
        if(mRaygenShader.Path.size() > 0)
        {
            new(&mRaygenShader.Module) ShaderModule(mContext, mRaygenShader.Path);
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_RAYGEN_BIT_KHR, mRaygenShader.Module);
            VkRayTracingShaderGroupCreateInfoKHR raygenGroupCreateInfo{};
            raygenGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            raygenGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            raygenGroupCreateInfo.generalShader      = static_cast<uint32_t>(shaderStageCreateInfos.Get()->size()) - 1;
            raygenGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
            raygenGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
            raygenGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            mShaderGroups.push_back(raygenGroupCreateInfo);
        }

        // Ray miss group
        if(mMissShader.Path.size() > 0)
        {
            new(&mMissShader.Module) ShaderModule(mContext, mMissShader.Path);
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_MISS_BIT_KHR, mMissShader.Module);
            VkRayTracingShaderGroupCreateInfoKHR missGroupCreateInfo{};
            missGroupCreateInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            missGroupCreateInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            missGroupCreateInfo.generalShader      = static_cast<uint32_t>(shaderStageCreateInfos.Get()->size()) - 1;
            missGroupCreateInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
            missGroupCreateInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
            missGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            mShaderGroups.push_back(missGroupCreateInfo);
        }

        // Ray closest hit group
        if(mClosesthitShader.Path.size() > 0)
        {
            new(&mClosesthitShader.Module) ShaderModule(mContext, mClosesthitShader.Path);
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, mClosesthitShader.Module);
            VkRayTracingShaderGroupCreateInfoKHR createInfo{};
            createInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            createInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            createInfo.generalShader      = VK_SHADER_UNUSED_KHR;
            createInfo.closestHitShader   = static_cast<uint32_t>(shaderStageCreateInfos.Get()->size()) - 1;
            createInfo.anyHitShader       = VK_SHADER_UNUSED_KHR;
            createInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            mShaderGroups.push_back(createInfo);
        }

        // Ray any hit group
        if(mAnyhitShader.Path.size() > 0)
        {
            new(&mAnyhitShader.Module) ShaderModule(mContext, mAnyhitShader.Path);
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, mAnyhitShader.Module);
            VkRayTracingShaderGroupCreateInfoKHR createInfo{};
            createInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            createInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            createInfo.generalShader      = VK_SHADER_UNUSED_KHR;
            createInfo.closestHitShader   = VK_SHADER_UNUSED_KHR;
            createInfo.anyHitShader       = static_cast<uint32_t>(shaderStageCreateInfos.Get()->size()) - 1;
            createInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            mShaderGroups.push_back(createInfo);
        }

        // Create the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{};
        raytracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracingPipelineCreateInfo.stageCount                   = static_cast<uint32_t>(shaderStageCreateInfos.Get()->size());
        raytracingPipelineCreateInfo.pStages                      = shaderStageCreateInfos.Get()->data();
        raytracingPipelineCreateInfo.groupCount                   = static_cast<uint32_t>(mShaderGroups.size());
        raytracingPipelineCreateInfo.pGroups                      = mShaderGroups.data();
        raytracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
        raytracingPipelineCreateInfo.layout                       = mPipelineLayout;
        AssertVkResult(mContext->DispatchTable.createRayTracingPipelinesKHR(VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline));
    }


    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytracingStage::GetAccelerationStructureDescriptorInfo(bool rebuild)
    {
        if(mAcclerationStructureDescriptorInfo != nullptr && !rebuild)
        {
            return mAcclerationStructureDescriptorInfo;
        }

        // Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
        mDescriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mDescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        mDescriptorAccelerationStructureInfo.pAccelerationStructures    = &mScene->GetComponent<TlasManager>()->GetTlas().GetAccelerationStructure();

        mAcclerationStructureDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mAcclerationStructureDescriptorInfo->Init(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        mAcclerationStructureDescriptorInfo->AddPNext(&mDescriptorAccelerationStructureInfo, 1);
        return mAcclerationStructureDescriptorInfo;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytracingStage::GetRenderTargetDescriptorInfo(bool rebuild)
    {
        if(mRenderTargetDescriptorInfo != nullptr && !rebuild)
        {
            return mRenderTargetDescriptorInfo;
        }

        UpdateRenderTargetDescriptorBufferInfos();

        mRenderTargetDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mRenderTargetDescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                                          &mRenderTargetDescriptorImageInfos);
        return mRenderTargetDescriptorInfo;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytracingStage::GetEnvironmentMapDescriptorInfo(bool rebuild)
    {
        if(!!mEnvironmentMapDescriptorInfo && !rebuild)
        {
            return mEnvironmentMapDescriptorInfo;
        }

        UpdateEnvironmentMapDescriptorInfos();

        mEnvironmentMapDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mEnvironmentMapDescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_MISS_BIT_KHR, &mEnvironmentMapDescriptorImageInfos);
        return mEnvironmentMapDescriptorInfo;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytracingStage::GetNoiseSourceDescriptorInfo(bool rebuild)
    {
        if(!!mNoiseSourceDescriptorInfo && !rebuild)
        {
            return mNoiseSourceDescriptorInfo;
        }

        UpdateNoiseSourceDescriptorInfos();

        mNoiseSourceDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mNoiseSourceDescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                                         &mNoiseSourceDescriptorImageInfos);
        return mNoiseSourceDescriptorInfo;
    }

    void RaytracingStage::UpdateRenderTargetDescriptorBufferInfos()
    {
        mRenderTargetDescriptorImageInfos.resize(1);
        mRenderTargetDescriptorImageInfos[0].imageView   = mRaytracingRenderTarget.GetImageView();
        mRenderTargetDescriptorImageInfos[0].imageLayout = mRaytracingRenderTarget.GetImageLayout();
    }

    void RaytracingStage::UpdateEnvironmentMapDescriptorInfos()
    {
        mEnvironmentMapDescriptorImageInfos = {VkDescriptorImageInfo{
            .sampler = mEnvironmentMapSampler, .imageView = mEnvironmentMap->GetImageView(), .imageLayout = mEnvironmentMap->GetImageLayout()}};  // namespace hsk
    }

    void RaytracingStage::UpdateNoiseSourceDescriptorInfos()
    {
        mNoiseSourceDescriptorImageInfos = {
            VkDescriptorImageInfo{.sampler = mNoiseSourceSampler, .imageView = mNoiseSource->GetImageView(), .imageLayout = mNoiseSource->GetImageLayout()}};  // namespace hsk
    }

    void RaytracingStage::SetupEnvironmentMap()
    {
        if(!mEnvironmentMapSampler && !!mEnvironmentMap)
        {
            VkSamplerCreateInfo samplerCi{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                          .magFilter               = VkFilter::VK_FILTER_LINEAR,
                                          .minFilter               = VkFilter::VK_FILTER_LINEAR,
                                          .addressModeU            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeV            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeW            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .anisotropyEnable        = VK_FALSE,
                                          .compareEnable           = VK_FALSE,
                                          .minLod                  = 0,
                                          .maxLod                  = 0,
                                          .unnormalizedCoordinates = VK_FALSE};
            AssertVkResult(vkCreateSampler(mContext->Device, &samplerCi, nullptr, &mEnvironmentMapSampler));
        }
    }
    void RaytracingStage::SetupNoiseSource()
    {
        if(!mNoiseSourceSampler && !!mNoiseSource)
        {
            VkSamplerCreateInfo samplerCi{.sType                   = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                          .magFilter               = VkFilter::VK_FILTER_NEAREST,
                                          .minFilter               = VkFilter::VK_FILTER_NEAREST,
                                          .addressModeU            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeV            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .addressModeW            = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          .anisotropyEnable        = VK_FALSE,
                                          .compareEnable           = VK_FALSE,
                                          .minLod                  = 0,
                                          .maxLod                  = 0,
                                          .unnormalizedCoordinates = VK_FALSE};
            AssertVkResult(vkCreateSampler(mContext->Device, &samplerCi, nullptr, &mNoiseSourceSampler));
        }
    }

}  // namespace hsk