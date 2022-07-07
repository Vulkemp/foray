#include "hsk_raytraycingstage.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../scenegraph/components/hsk_camera.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/globalcomponents/hsk_materialbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_texturestore.hpp"
#include "../utility/hsk_pipelinebuilder.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "../utility/hsk_shaderstagecreateinfos.hpp"


namespace hsk {
    void RaytraycingStage::Init(const VkContext* context, Scene* scene)
    {
        mContext = context;
        mScene   = scene;

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

    void RaytraycingStage::CreateFixedSizeComponents()
    {
        SetupDescriptors();
        PreparePipeline();
    }

    void RaytraycingStage::DestroyFixedComponents()
    {
        VkDevice device = mContext->Device;
        if(mPipeline)
        {
            vkDestroyPipeline(device, mPipeline, nullptr);
            mPipeline = nullptr;
        }
        if(mPipelineLayout)
        {
            vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
       /* if(mPipelineCache)
        {
            vkDestroyPipelineCache(device, mPipelineCache, nullptr);
            mPipelineCache = nullptr;
        }*/
        mDescriptorSet.Cleanup();
    }

    void RaytraycingStage::CreateResolutionDependentComponents()
    {
        PrepareAttachments();
        PrepareRenderpass();
        BuildCommandBuffer();
    }

    void RaytraycingStage::DestroyResolutionDependentComponents()
    {
        VkDevice device = mContext->Device;
        for(auto& colorAttachment : mColorAttachments)
        {
            colorAttachment->Cleanup();
        }
        mDepthAttachment.Cleanup();
        /*if(mFrameBuffer)
        {
            vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
        if(mRenderpass)
        {
            vkDestroyRenderPass(device, mRenderpass, nullptr);
            mRenderpass = nullptr;
        }*/
    }

    void RaytraycingStage::PrepareAttachments()
    {
        static const VkFormat          colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->Swapchain.extent.width, mContext->Swapchain.extent.height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageLayout            intialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;


        // ---
        // can we create an image with intial layout general? VK_IMAGE_LAYOUT_GENERAL
        mRaytracingRenderTarget.Create(mContext, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocationCreateFlags, extent, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                       VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, RaytracingRenderTargetName);

        mColorAttachments.reserve(mGBufferImages.size());
        for(size_t i = 0; i < mGBufferImages.size(); i++)
        {
            mColorAttachments.push_back(mGBufferImages[i].get());
        }

        mDepthAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_ASPECT_DEPTH_BIT, "GBuffer_DepthBufferImage");
    }

    void RaytraycingStage::PrepareRenderpass()
    {
        // size + 1 for depth attachment description
        std::vector<VkAttachmentDescription> attachmentDescriptions(mColorAttachments.size() + 1);
        std::vector<VkAttachmentReference>   colorAttachmentReferences(mColorAttachments.size());
        std::vector<VkImageView>             attachmentViews(attachmentDescriptions.size());

        for(uint32_t i = 0; i < mColorAttachments.size(); i++)
        {
            auto& colorAttachment                    = mColorAttachments[i];
            attachmentDescriptions[i].samples        = colorAttachment->GetSampleCount();
            attachmentDescriptions[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescriptions[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescriptions[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescriptions[i].finalLayout    = VK_IMAGE_LAYOUT_GENERAL;
            attachmentDescriptions[i].format         = colorAttachment->GetFormat();

            colorAttachmentReferences[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            attachmentViews[i]           = colorAttachment->GetImageView();
        }

        // prepare depth attachment
        VkAttachmentDescription depthAttachmentDescription{};
        depthAttachmentDescription.samples        = mDepthAttachment.GetSampleCount();
        depthAttachmentDescription.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDescription.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachmentDescription.format         = mDepthAttachment.GetFormat();

        // the depth attachment gets the final id (one higher than the highest color attachment id)
        VkAttachmentReference depthAttachmentReference   = {(uint32_t)colorAttachmentReferences.size(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        attachmentDescriptions[mColorAttachments.size()] = depthAttachmentDescription;
        attachmentViews[mColorAttachments.size()]        = mDepthAttachment.GetImageView();

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentReferences.size();
        subpass.pColorAttachments       = colorAttachmentReferences.data();
        subpass.pDepthStencilAttachment = &depthAttachmentReference;

        VkSubpassDependency subPassDependencies[2] = {};
        subPassDependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
        subPassDependencies[0].dstSubpass          = 0;
        subPassDependencies[0].srcStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependencies[0].srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subPassDependencies[0].dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;

        subPassDependencies[1].srcSubpass      = 0;
        subPassDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        subPassDependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subPassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pAttachments           = attachmentDescriptions.data();
        renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;
        /*      AssertVkResult(vkCreateRenderPass(mContext->Device, &renderPassInfo, nullptr, &mRenderpass));

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = static_cast<uint32_t>(attachmentViews.size());
        fbufCreateInfo.width                   = mContext->Swapchain.extent.width;
        fbufCreateInfo.height                  = mContext->Swapchain.extent.height;
        fbufCreateInfo.layers                  = 1;
        AssertVkResult(vkCreateFramebuffer(mContext->Device, &fbufCreateInfo, nullptr, &mFrameBuffer));*/
    }

    void RaytraycingStage::SetupDescriptors()
    {
        mDescriptorSet.SetDescriptorInfoAt(0, GetAccelerationStructureDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(1, GetRenderTargetDescriptorInfo());
        std::vector<Node*> nodes;
        mScene->FindNodesWithComponent<Camera>(nodes);
        mDescriptorSet.SetDescriptorInfoAt(2, nodes.front()->GetComponent<Camera>()->GetUboDescriptorInfos());

        VkDescriptorSetLayout descriptorSetLayout = mDescriptorSet.Create(mContext, "RaytraycingPipelineDescriptorSet");


        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts    = &mDescriptorSet.GetDescriptorSetLayout();
        AssertVkResult(vkCreatePipelineLayout(mContext->Device, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout));
    }

    void RaytraycingStage::RecordFrame(FrameRenderInfo& renderInfo)
    {
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        //renderPassBeginInfo.renderPass        = mRenderpass;
        //renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mContext->Swapchain.extent;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(mClearValues.size());
        renderPassBeginInfo.pClearValues      = mClearValues.data();

        VkCommandBuffer commandBuffer = renderInfo.GetCommandBuffer();
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // = vks::initializers::viewport((float)mRenderResolution.width, (float)mRenderResolution.height, 0.0f, 1.0f);
        VkViewport viewport{0.f, 0.f, (float)mContext->Swapchain.extent.width, (float)mContext->Swapchain.extent.height, 0.0f, 1.0f};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->Swapchain.extent.width, mContext->Swapchain.extent.height}};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        const auto& descriptorsets = mDescriptorSet.GetDescriptorSets();

        // Instanced object
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &(descriptorsets[(renderInfo.GetFrameNumber()) % 2]), 0, nullptr);
        mScene->Draw(renderInfo, mPipelineLayout);  // TODO: does pipeline has to be passed? Technically a scene could build pipelines themselves.

        vkCmdEndRenderPass(commandBuffer);
    }

    void RaytraycingStage::PreparePipeline()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        //AssertVkResult(vkCreatePipelineCache(mContext->Device, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

        // shader stages
        auto                   vertShaderModule = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/gbuffer_stage.vert.spv");
        auto                   fragShaderModule = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/gbuffer_stage.frag.spv");
        ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule).Add(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);

        // vertex layout
        VertexInputStateBuilder vertexInputStateBuilder;
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Position);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Normal);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Tangent);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Uv);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::MaterialIndex);
        vertexInputStateBuilder.Build();

        // clang-format off
        mPipeline = PipelineBuilder()
            .SetContext(mContext)
            // Blend attachment states required for all color attachments
            // This is important, as color write mask will otherwise be 0x0 and you
            // won't see anything rendered to the attachment
            .SetColorAttachmentBlendCount(mColorAttachments.size())
            .SetPipelineLayout(mPipelineLayout)
            .SetVertexInputStateBuilder(&vertexInputStateBuilder)
            .SetShaderStageCreateInfos(shaderStageCreateInfos.Get())
            //.SetPipelineCache(mPipelineCache)
            //.SetRenderPass(mRenderpass)
            .Build();
        // clang-format on
    }

    void RaytraycingStage::CreateShaderBindingTables()
    {
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
        mRaygenShaderBindingTable = std::make_unique<ManagedBuffer>();
        mMissShaderBindingTable   = std::make_unique<ManagedBuffer>();
        mHitShaderBindingTable    = std::make_unique<ManagedBuffer>();

        mRaygenShaderBindingTable->Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags);
        mMissShaderBindingTable->Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags);
        mHitShaderBindingTable->Create(mContext, sbtBufferUsageFlags, sbtSize, sbtMemoryFlags, sbt_allocation_create_flags);


        // Copy the pipeline's shader handles into a host buffer
        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        AssertVkResult(vkGetRayTracingShaderGroupHandlesKHR(mContext->Device, mPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

        // Copy the shader handles from the host buffer to the binding tables
        mRaygenShaderBindingTable->MapAndWrite(shaderHandleStorage.data(), handleSize);
        mMissShaderBindingTable->MapAndWrite(shaderHandleStorage.data() + handleSizeAligned, handleSize);
        mHitShaderBindingTable->MapAndWrite(shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
    }

    void RaytraycingStage::CreateRaytraycingPipeline()
    {
        // Setup ray tracing shader groups
        // Each shader group points at the corresponding shader in the pipeline

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

        // shader stages
        auto                   rgenShaderModule  = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/raytracing/raygen.rgen.spv");
        auto                   rmissShaderModule = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/raytracing/miss.rmiss.spv");
        auto                   rchitShaderModule = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/raytracing/closesthit.rchit.spv");
        ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_RAYGEN_BIT_KHR, rgenShaderModule);


        // Ray generation group
        {
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_RAYGEN_BIT_KHR, rgenShaderModule);
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
        {
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_MISS_BIT_KHR, rmissShaderModule);
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
        {
            shaderStageCreateInfos.Add(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, rchitShaderModule);
            VkRayTracingShaderGroupCreateInfoKHR closes_hit_group_ci{};
            closes_hit_group_ci.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            closes_hit_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            closes_hit_group_ci.generalShader      = VK_SHADER_UNUSED_KHR;
            closes_hit_group_ci.closestHitShader   = static_cast<uint32_t>(shaderStageCreateInfos.Get()->size()) - 1;
            closes_hit_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
            closes_hit_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
            mShaderGroups.push_back(closes_hit_group_ci);
        }

        // Create the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{};
        raytracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracingPipelineCreateInfo.stageCount                   = static_cast<uint32_t>(shader_stages.size());
        raytracingPipelineCreateInfo.pStages                      = shader_stages.data();
        raytracingPipelineCreateInfo.groupCount                   = static_cast<uint32_t>(mShaderGroups.size());
        raytracingPipelineCreateInfo.pGroups                      = mShaderGroups.data();
        raytracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
        raytracingPipelineCreateInfo.layout                       = mPipelineLayout;
        AssertVkResult(vkCreateRayTracingPipelinesKHR(mContext->Device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline));
    }


    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytraycingStage::GetAccelerationStructureDescriptorInfo()
    {
        if(mAcclerationStructureDescriptorInfo != nullptr)
        {
            return mAcclerationStructureDescriptorInfo;
        }

        // Setup the descriptor for binding our top level acceleration structure to the ray tracing shaders
        mDescriptorAccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mDescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        mDescriptorAccelerationStructureInfo.pAccelerationStructures    = &mTopLevelAccelerationStructure;

        mAcclerationStructureDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mAcclerationStructureDescriptorInfo->Init(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
        mAcclerationStructureDescriptorInfo->AddPNext(&mDescriptorAccelerationStructureInfo);
        return mAcclerationStructureDescriptorInfo;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> RaytraycingStage::GetRenderTargetDescriptorInfo()
    {

        VkDescriptorSetLayoutBinding uniformBufferBinding{};
        uniformBufferBinding.binding         = 2;
        uniformBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferBinding.descriptorCount = 1;
        uniformBufferBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        if(mRenderTargetDescriptorInfo != nullptr)
        {
            return mRenderTargetDescriptorInfo;
        }

        UpdateRenderTargetDescriptorBufferInfos();

        mRenderTargetDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mRenderTargetDescriptorInfo->Init(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                                          &mRenderTargetDescriptorImageInfos);
        return mRenderTargetDescriptorInfo;
    }

    void RaytraycingStage::UpdateRenderTargetDescriptorBufferInfos()
    {
        mRenderTargetDescriptorImageInfos[0].imageView   = mRaytracingRenderTarget.GetImageView();
        mRenderTargetDescriptorImageInfos[0].imageLayout = mRaytracingRenderTarget.GetImageLayout();
    }

}  // namespace hsk