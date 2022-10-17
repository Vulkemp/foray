#include "foray_gbuffer.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/components/foray_meshinstance.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_drawdirector.hpp"
#include "../scene/globalcomponents/foray_geometrystore.hpp"
#include "../scene/globalcomponents/foray_materialbuffer.hpp"
#include "../scene/globalcomponents/foray_texturestore.hpp"
#include "../util/foray_pipelinebuilder.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"

#ifdef ENABLE_GBUFFER_BENCH
#pragma message "Gbuffer Benching enabled. Added synchronisation scopes may cause reduced performance!"
#endif  // ENABLE_GBUFFER_BENCH

namespace foray::stages {
    // Heavily inspired from Sascha Willems' "deferred" vulkan example
    void GBufferStage::Init(const core::VkContext* context, scene::Scene* scene)
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

    void GBufferStage::CreateFixedSizeComponents()
    {
#ifdef ENABLE_GBUFFER_BENCH
        std::vector<const char*> timestampNames(
            {bench::BenchmarkTimestamp::BEGIN, TIMESTAMP_VERT_BEGIN, TIMESTAMP_VERT_END, TIMESTAMP_FRAG_BEGIN, TIMESTAMP_FRAG_END, bench::BenchmarkTimestamp::END});
        mBenchmark.Create(mContext, timestampNames);
#endif  // ENABLE_GBUFFER_BENCH
        SetupDescriptors();
        PreparePipeline();
    }

    void GBufferStage::DestroyFixedComponents()
    {
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.Destroy();
#endif  // ENABLE_GBUFFER_BENCH
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
        if(mPipelineCache)
        {
            vkDestroyPipelineCache(device, mPipelineCache, nullptr);
            mPipelineCache = nullptr;
        }
        mDescriptorSet.Destroy();
    }

    void GBufferStage::CreateResolutionDependentComponents()
    {
        PrepareAttachments();
        PrepareRenderpass();
        BuildCommandBuffer();
    }

    void GBufferStage::DestroyResolutionDependentComponents()
    {
        VkDevice device = mContext->Device;
        for(auto& colorAttachment : mColorAttachments)
        {
            colorAttachment->Destroy();
        }
        mDepthAttachment.Destroy();
        if(mFrameBuffer)
        {
            vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
        if(mRenderpass)
        {
            vkDestroyRenderPass(device, mRenderpass, nullptr);
            mRenderpass = nullptr;
        }
    }

    void GBufferStage::OnShadersRecompiled()
    {
        // check if shaders have been recompiled
        //bool needsPipelineUpdate = shaderCompiler->HasShaderBeenRecompiled(mVertexShaderPath) || shaderCompiler->HasShaderBeenRecompiled(mFragmentShaderPath);
        bool needsPipelineUpdate =
            core::ShaderManager::Instance().HasShaderBeenRecompiled(mVertexShaderPath) || core::ShaderManager::Instance().HasShaderBeenRecompiled(mFragmentShaderPath);

        if(!needsPipelineUpdate)
            return;

        vkDeviceWaitIdle(mContext->Device);
        // rebuild pipeline and its dependencies.
        PreparePipeline();
    }

    void GBufferStage::PrepareAttachments()
    {
        static const VkFormat colorFormat    = VK_FORMAT_R16G16B16A16_SFLOAT;
        static const VkFormat geometryFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->Swapchain.extent.width, mContext->Swapchain.extent.height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;
        VkImageLayout            intialLayout          = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags       aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;

        mGBufferImages.clear();
        mGBufferImages.reserve(6);
        mGBufferImages.push_back(std::make_unique<core::ManagedImage>());
        mGBufferImages.push_back(std::make_unique<core::ManagedImage>());
        mGBufferImages.push_back(std::make_unique<core::ManagedImage>());
        mGBufferImages.push_back(std::make_unique<core::ManagedImage>());
        mGBufferImages.push_back(std::make_unique<core::ManagedImage>());
        mGBufferImages.push_back(std::make_unique<core::ManagedImage>());
        mGBufferImages[0]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat, intialLayout, aspectMask, WorldspacePosition);
        mGBufferImages[1]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat, intialLayout, aspectMask, WorldspaceNormal);
        mGBufferImages[2]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat, intialLayout, aspectMask, Albedo);
        mGBufferImages[3]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat, intialLayout, aspectMask, MotionVector);
        mGBufferImages[4]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, VK_FORMAT_R32_SINT, intialLayout, aspectMask, MaterialIndex);
        mGBufferImages[5]->Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, VK_FORMAT_R32_UINT, intialLayout, aspectMask, MeshInstanceIndex);

        mColorAttachments.clear();
        mColorAttachments.reserve(mGBufferImages.size());
        for(size_t i = 0; i < mGBufferImages.size(); i++)
        {
            mColorAttachments.push_back(mGBufferImages[i].get());
        }

        mDepthAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_ASPECT_DEPTH_BIT, "GBuffer_DepthBufferImage");

        {  // Pre-transfer to correct layout
            core::HostCommandBuffer commandBuffer;
            commandBuffer.Create(mContext);
            commandBuffer.Begin();

            std::vector<VkImageMemoryBarrier2> barriers;
            barriers.reserve(6);

            VkImageMemoryBarrier2 attachmentMemBarrier{
                .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount     = VK_REMAINING_ARRAY_LAYERS,
                    },
            };

            for(std::unique_ptr<core::ManagedImage>& image : mGBufferImages)
            {
                attachmentMemBarrier.image = image->GetImage();
                barriers.push_back(attachmentMemBarrier);
            }

            barriers.push_back(VkImageMemoryBarrier2{
                .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask       = VK_ACCESS_2_NONE,
                .dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = mDepthAttachment.GetImage(),
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount     = VK_REMAINING_ARRAY_LAYERS,
                    },
            });

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = (uint32_t)barriers.size(), .pImageMemoryBarriers = barriers.data()};

            vkCmdPipelineBarrier2(commandBuffer, &depInfo);

            commandBuffer.SubmitAndWait();
        }
    }

    void GBufferStage::PrepareRenderpass()
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
        depthAttachmentDescription.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachmentDescription.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
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
        AssertVkResult(vkCreateRenderPass(mContext->Device, &renderPassInfo, nullptr, &mRenderpass));

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = static_cast<uint32_t>(attachmentViews.size());
        fbufCreateInfo.width                   = mContext->Swapchain.extent.width;
        fbufCreateInfo.height                  = mContext->Swapchain.extent.height;
        fbufCreateInfo.layers                  = 1;
        AssertVkResult(vkCreateFramebuffer(mContext->Device, &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void GBufferStage::SetupDescriptors()
    {
        mDescriptorSet.SetDescriptorInfoAt(0, mScene->GetComponent<scene::MaterialBuffer>()->GetDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(1, mScene->GetComponent<scene::TextureStore>()->GetDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(2, mScene->GetComponent<scene::CameraManager>()->MakeUboDescriptorInfos());
        mDescriptorSet.SetDescriptorInfoAt(3, mScene->GetComponent<scene::DrawDirector>()->MakeDescriptorInfosForCurrent());
        mDescriptorSet.SetDescriptorInfoAt(4, mScene->GetComponent<scene::DrawDirector>()->MakeDescriptorInfosForPrevious());

        VkDescriptorSetLayout descriptorSetLayout = mDescriptorSet.Create(mContext, "GBuffer_DescriptorSet");

        std::vector<VkPushConstantRange> pushConstantRanges({{.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
                                                              .offset     = 0,
                                                              .size       = sizeof(scene::DrawPushConstant)}});

        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutCI.pPushConstantRanges    = pushConstantRanges.data();
        pipelineLayoutCI.setLayoutCount         = 1;
        pipelineLayoutCI.pSetLayouts            = &descriptorSetLayout;

        AssertVkResult(vkCreatePipelineLayout(mContext->Device, &pipelineLayoutCI, nullptr, &mPipelineLayout));
    }

    void GBufferStage::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {

        uint32_t frameNum = renderInfo.GetFrameNumber();
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.CmdResetQuery(commandBuffer, frameNum);
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, bench::BenchmarkTimestamp::BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
#endif  // ENABLE_GBUFFER_BENCH

        VkImageMemoryBarrier2 attachmentMemBarrier{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask       = VK_ACCESS_2_NONE,
            .dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout           = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask     = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
        };

        std::vector<VkImageMemoryBarrier2> imgBarriers;
        imgBarriers.reserve(mGBufferImages.size() + 1);

        for(std::unique_ptr<core::ManagedImage>& image : mGBufferImages)
        {
            attachmentMemBarrier.image = image->GetImage();
            imgBarriers.push_back(attachmentMemBarrier);
        }
        attachmentMemBarrier.dstAccessMask               = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        attachmentMemBarrier.dstStageMask                = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        attachmentMemBarrier.newLayout                   = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        attachmentMemBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
        attachmentMemBarrier.image                       = mDepthAttachment.GetImage();
        imgBarriers.push_back(attachmentMemBarrier);

        std::vector<VkBufferMemoryBarrier2> bufferBarriers;

        VkBufferMemoryBarrier2 bufferBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                             .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                             .srcAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                             .dstStageMask        = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                                             .dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT,
                                             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                             .offset              = 0,
                                             .size                = VK_WHOLE_SIZE};

        bufferBarrier.buffer = mScene->GetComponent<scene::MaterialBuffer>()->GetBuffer().GetBuffer().GetBuffer();
        bufferBarriers.push_back(bufferBarrier);
        bufferBarrier.buffer = mScene->GetComponent<scene::CameraManager>()->GetUbo().GetUboBuffer().GetDeviceBuffer().GetBuffer();
        bufferBarriers.push_back(bufferBarrier);
        bufferBarrier.buffer = mScene->GetComponent<scene::DrawDirector>()->GetCurrentTransformBuffer().GetDeviceBuffer().GetBuffer();
        bufferBarriers.push_back(bufferBarrier);
        bufferBarrier.buffer = mScene->GetComponent<scene::DrawDirector>()->GetPreviousTransformBuffer().GetBuffer();
        bufferBarriers.push_back(bufferBarrier);

        VkDependencyInfo depInfo{
            .sType                    = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags          = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT,
            .bufferMemoryBarrierCount = (uint32_t)bufferBarriers.size(),
            .pBufferMemoryBarriers    = bufferBarriers.data(),
            .imageMemoryBarrierCount  = (uint32_t)imgBarriers.size(),
            .pImageMemoryBarriers     = imgBarriers.data(),
        };

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mContext->Swapchain.extent;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(mClearValues.size());
        renderPassBeginInfo.pClearValues      = mClearValues.data();

        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // = vks::initializers::viewport((float)mRenderResolution.width, (float)mRenderResolution.height, 0.0f, 1.0f);
        VkViewport viewport{0.f, 0.f, (float)mContext->Swapchain.extent.width, (float)mContext->Swapchain.extent.height, 0.0f, 1.0f};
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->Swapchain.extent.width, mContext->Swapchain.extent.height}};
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        const auto& descriptorsets = mDescriptorSet.GetDescriptorSets();

        // Instanced object
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &(descriptorsets[(renderInfo.GetFrameNumber()) % descriptorsets.size()]), 0,
                                nullptr);

        auto bit = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_VERT_BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        mBenchmark.CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_VERT_END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
        mBenchmark.CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_FRAG_BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        mBenchmark.CmdWriteTimestamp(cmdBuffer, frameNum, TIMESTAMP_FRAG_END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
#endif  // ENABLE_GBUFFER_BENCH

        mScene->Draw(renderInfo, mPipelineLayout, cmdBuffer);  // TODO: does pipeline has to be passed? Technically a scene could build pipelines themselves.

        vkCmdEndRenderPass(cmdBuffer);
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.CmdWriteTimestamp(cmdBuffer, frameNum, bench::BenchmarkTimestamp::END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
#endif  // ENABLE_GBUFFER_BENCH
    }

    void GBufferStage::PreparePipeline()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        AssertVkResult(vkCreatePipelineCache(mContext->Device, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

        // shader stages
        auto                         vertShaderModule = core::ShaderModule(mContext, mVertexShaderPath);
        auto                         fragShaderModule = core::ShaderModule(mContext, mFragmentShaderPath);
        util::ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule).Add(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);

        // vertex layout
        scene::VertexInputStateBuilder vertexInputStateBuilder;
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Position);
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Normal);
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Tangent);
        vertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Uv);
        vertexInputStateBuilder.Build();

        // clang-format off
        mPipeline = util::PipelineBuilder()
            .SetContext(mContext)
            // Blend attachment states required for all color attachments
            // This is important, as color write mask will otherwise be 0x0 and you
            // won't see anything rendered to the attachment
            .SetColorAttachmentBlendCount(mColorAttachments.size())
            .SetPipelineLayout(mPipelineLayout)
            .SetVertexInputStateBuilder(&vertexInputStateBuilder)
            .SetShaderStageCreateInfos(shaderStageCreateInfos.Get())
            .SetPipelineCache(mPipelineCache)
            .SetRenderPass(mRenderpass)
            .Build();
        // clang-format on
    }
}  // namespace foray::stages