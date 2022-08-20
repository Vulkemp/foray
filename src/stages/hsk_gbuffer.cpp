#include "hsk_gbuffer.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/globalcomponents/hsk_cameramanager.hpp"
#include "../scenegraph/globalcomponents/hsk_drawdirector.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/globalcomponents/hsk_materialbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_texturestore.hpp"
#include "../utility/hsk_pipelinebuilder.hpp"
#include "../utility/hsk_shadermodule.hpp"
#include "../utility/hsk_shaderstagecreateinfos.hpp"

#ifdef ENABLE_GBUFFER_BENCH
#pragma message "Gbuffer Benching enabled. Added synchronisation scopes may cause reduced performance!"
#endif  // ENABLE_GBUFFER_BENCH

namespace hsk {
    // Heavily inspired from Sascha Willems' "deferred" vulkan example
    void GBufferStage::Init(const VkContext* context, Scene* scene)
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
            {BenchmarkTimestamp::BEGIN, TIMESTAMP_VERT_BEGIN, TIMESTAMP_VERT_END, TIMESTAMP_FRAG_BEGIN, TIMESTAMP_FRAG_END, BenchmarkTimestamp::END});
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

    void GBufferStage::OnShadersRecompiled(ShaderCompiler* shaderCompiler)
    {
        // check if shaders have been recompiled
        bool needsPipelineUpdate = shaderCompiler->HasShaderBeenRecompiled(mVertexShaderPath) || shaderCompiler->HasShaderBeenRecompiled(mFragmentShaderPath);
        if(!needsPipelineUpdate)
            return;

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
        mGBufferImages.push_back(std::make_unique<ManagedImage>());
        mGBufferImages.push_back(std::make_unique<ManagedImage>());
        mGBufferImages.push_back(std::make_unique<ManagedImage>());
        mGBufferImages.push_back(std::make_unique<ManagedImage>());
        mGBufferImages.push_back(std::make_unique<ManagedImage>());
        mGBufferImages.push_back(std::make_unique<ManagedImage>());
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
        mDescriptorSet.SetDescriptorInfoAt(0, mScene->GetComponent<MaterialBuffer>()->GetDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(1, mScene->GetComponent<TextureStore>()->GetDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(2, mScene->GetComponent<CameraManager>()->MakeUboDescriptorInfos());
        mDescriptorSet.SetDescriptorInfoAt(3, mScene->GetComponent<DrawDirector>()->MakeDescriptorInfosForCurrent());
        mDescriptorSet.SetDescriptorInfoAt(4, mScene->GetComponent<DrawDirector>()->MakeDescriptorInfosForPrevious());

        VkDescriptorSetLayout descriptorSetLayout = mDescriptorSet.Create(mContext, "GBuffer_DescriptorSet");

        std::vector<VkPushConstantRange> pushConstantRanges({{.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
                                                              .offset     = 0,
                                                              .size       = sizeof(DrawPushConstant)}});

        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutCI.pPushConstantRanges    = pushConstantRanges.data();
        pipelineLayoutCI.setLayoutCount         = 1;
        pipelineLayoutCI.pSetLayouts            = &descriptorSetLayout;

        AssertVkResult(vkCreatePipelineLayout(mContext->Device, &pipelineLayoutCI, nullptr, &mPipelineLayout));
    }

    void GBufferStage::RecordFrame(FrameRenderInfo& renderInfo)
    {

        VkCommandBuffer commandBuffer = renderInfo.GetCommandBuffer();
        uint32_t        frameNum      = renderInfo.GetFrameNumber();
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.CmdResetQuery(commandBuffer, frameNum);
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, BenchmarkTimestamp::BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
#endif  // ENABLE_GBUFFER_BENCH

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mContext->Swapchain.extent;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(mClearValues.size());
        renderPassBeginInfo.pClearValues      = mClearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // = vks::initializers::viewport((float)mRenderResolution.width, (float)mRenderResolution.height, 0.0f, 1.0f);
        VkViewport viewport{0.f, 0.f, (float)mContext->Swapchain.extent.width, (float)mContext->Swapchain.extent.height, 0.0f, 1.0f};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mContext->Swapchain.extent.width, mContext->Swapchain.extent.height}};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        const auto& descriptorsets = mDescriptorSet.GetDescriptorSets();

        // Instanced object
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &(descriptorsets[(renderInfo.GetFrameNumber()) % descriptorsets.size()]), 0,
                                nullptr);

        auto bit = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, TIMESTAMP_VERT_BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, TIMESTAMP_VERT_END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, TIMESTAMP_FRAG_BEGIN, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, TIMESTAMP_FRAG_END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
#endif  // ENABLE_GBUFFER_BENCH

        mScene->Draw(renderInfo, mPipelineLayout);  // TODO: does pipeline has to be passed? Technically a scene could build pipelines themselves.

        vkCmdEndRenderPass(commandBuffer);
#ifdef ENABLE_GBUFFER_BENCH
        mBenchmark.CmdWriteTimestamp(commandBuffer, frameNum, BenchmarkTimestamp::END, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
#endif  // ENABLE_GBUFFER_BENCH
    }

    void GBufferStage::PreparePipeline()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        AssertVkResult(vkCreatePipelineCache(mContext->Device, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

        // shader stages
        auto                   vertShaderModule = ShaderModule(mContext, mVertexShaderPath);
        auto                   fragShaderModule = ShaderModule(mContext, mFragmentShaderPath);
        ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule).Add(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);

        // vertex layout
        VertexInputStateBuilder vertexInputStateBuilder;
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Position);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Normal);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Tangent);
        vertexInputStateBuilder.AddVertexComponentBinding(EVertexComponent::Uv);
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
            .SetPipelineCache(mPipelineCache)
            .SetRenderPass(mRenderpass)
            .Build();
        // clang-format on
    }
}  // namespace hsk