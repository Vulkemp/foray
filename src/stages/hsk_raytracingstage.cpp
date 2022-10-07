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
#include "../utility/hsk_shadermanager.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "../utility/hsk_shaderstagecreateinfos.hpp"
#include <array>

namespace hsk {

    void RaytracingStage::Init()
    {
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
        SetupDescriptors();
        CreateDescriptorSets();
        CreatePipelineLayout();
        CreateRaytraycingPipeline();
    }

    void RaytracingStage::DestroyFixedComponents()
    {
        DestroyShaders();
        mPipeline.Destroy();
        if(!!mContext)
        {
            VkDevice device = mContext->Device;
            if(!!mPipelineLayout)
            {
                vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
                mPipelineLayout = nullptr;
            }
        }
        mDescriptorSet.Destroy();
        if(mEnvMap.IsSet)
        {
            mEnvMap.Destroy(mContext);
        }
        if(mNoiseSource.IsSet)
        {
            mNoiseSource.Destroy(mContext);
        }
    }

    void RaytracingStage::CreateResolutionDependentComponents()
    {
        PrepareAttachments();
    }

    void RaytracingStage::DestroyResolutionDependentComponents()
    {
        mColorAttachments.clear();
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
        if(mEnvMap.IsSet)
        {
            mDescriptorSet.SetDescriptorInfoAt(9, mEnvMap.GetDescriptorInfo());
        }
        if(mNoiseSource.IsSet)
        {
            mDescriptorSet.SetDescriptorInfoAt(10, mNoiseSource.GetDescriptorInfo());
        } 
    }

    void RaytracingStage::CreateDescriptorSets() {
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

        mPipeline.CmdBindPipeline(commandBuffer);

        const auto& descriptorsets = mDescriptorSet.GetDescriptorSets();
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineLayout, 0, 1,
                                &(descriptorsets[(renderInfo.GetFrameNumber()) % descriptorsets.size()]), 0, nullptr);

        mPushConstant.RngSeed = renderInfo.GetFrameNumber();
        vkCmdPushConstants(commandBuffer, mPipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0,
                           sizeof(mPushConstant), &mPushConstant);

        VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry = mPipeline.GetRaygenSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry = mPipeline.GetMissSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry = mPipeline.GetHitSbt().GetAddressRegion();

        VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry = mPipeline.GetCallablesSbt().GetAddressRegion();

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->Swapchain.extent.width, mContext->Swapchain.extent.height}};
        mContext->DispatchTable.cmdTraceRaysKHR(commandBuffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry,
                                                scissor.extent.width, scissor.extent.height, 1);
    }

    void RaytracingStage::ReloadShaders()
    {
        vkDeviceWaitIdle(mContext->Device);

        DestroyShaders();
        mPipeline.Destroy();
        CreateRaytraycingPipeline();
    }

    void RaytracingStage::CreateRaytraycingPipeline()
    {
        mPipeline.Build(mContext, mPipelineLayout);
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

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytracingStage::SampledImage::GetDescriptorInfo(bool rebuild)
    {
        if(!!DescriptorInfo && !rebuild)
        {
            return DescriptorInfo;
        }

        UpdateDescriptorInfos();

        DescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        DescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RTSTAGEFLAGS, &DescriptorImageInfos);
        return DescriptorInfo;
    }

    void RaytracingStage::UpdateRenderTargetDescriptorBufferInfos()
    {
        mRenderTargetDescriptorImageInfos.resize(1);
        mRenderTargetDescriptorImageInfos[0].imageView   = mRaytracingRenderTarget.GetImageView();
        mRenderTargetDescriptorImageInfos[0].imageLayout = mRaytracingRenderTarget.GetImageLayout();
    }

    void RaytracingStage::SampledImage::UpdateDescriptorInfos()
    {
        DescriptorImageInfos = {VkDescriptorImageInfo{.sampler = Sampler, .imageView = Image->GetImageView(), .imageLayout = Image->GetImageLayout()}};  // namespace hsk
    }

    void RaytracingStage::SampledImage::Create(const VkContext* context, ManagedImage* image, bool initateSampler)
    {
        Image = image;
        if(!!Image && initateSampler)
        {
            if(!!Sampler)
            {
                vkDestroySampler(context->Device, Sampler, nullptr);
            }
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
            AssertVkResult(vkCreateSampler(context->Device, &samplerCi, nullptr, &Sampler));
        }
        IsSet = !!Image;
    }

    void RaytracingStage::SampledImage::Destroy(const VkContext* context)
    {
        if(!!Sampler)
        {
            vkDestroySampler(context->Device, Sampler, nullptr);
            Sampler = nullptr;
        }
        Image = nullptr;
        IsSet = false;
        DescriptorImageInfos.clear();
        DescriptorInfo = nullptr;
    }

}  // namespace hsk