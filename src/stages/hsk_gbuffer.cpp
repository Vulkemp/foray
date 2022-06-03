#include "hsk_gbuffer.hpp"
#include "../glTF/hsk_geo.hpp"
#include "../glTF/hsk_scenedrawinfo.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../utility/hsk_shadermodule.hpp"

namespace hsk {
    // Heavily inspired from Sascha Willems' "deferred" vulkan example
    void GBufferStage::Init(const VkContext* context, Scene* scene)
    {
        mContext = context;
        mScene   = scene;

        mScene->AssertSceneloaded(true);
        // declare all descriptors here

        InitResolutionDependentComponents();
        InitFixedSizeComponents();
    }

    void GBufferStage::InitFixedSizeComponents()
    {
        SetupDescriptors();
        PreparePipeline();
    }
    void GBufferStage::InitResolutionDependentComponents()
    {
        PrepareAttachments();
        PrepareRenderpass();
        BuildCommandBuffer();
    }
    void GBufferStage::DestroyResolutionDependentComponents() {}

    void GBufferStage::PrepareAttachments()
    {
        static const VkFormat colorFormat    = VK_FORMAT_R16G16B16A16_SFLOAT;
        static const VkFormat geometryFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        static const VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkExtent3D               extent                = {mContext->Swapchain.extent.width, mContext->Swapchain.extent.height, 1};
        VmaMemoryUsage           memoryUsage           = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationCreateFlags allocationCreateFlags = 0;

        // if res = result false, image format with given usage flags is not supported.
        //VkImageFormatProperties props{};
        //auto res = vkGetPhysicalDeviceImageFormatProperties(mContext->PhysicalDevice, VK_FORMAT_R8G8B8A8_SRGB, VkImageType::VK_IMAGE_TYPE_2D, VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
        //imageUsageFlags, 0, &props);

        mPositionAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat);
        mNormalAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat);
        mAlbedoAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat);
        mMotionAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, colorFormat);
        mMeshIdAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, imageUsageFlags, VK_FORMAT_R32_SINT);
        mDepthAttachment.Create(mContext, memoryUsage, allocationCreateFlags, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    void GBufferStage::PrepareRenderpass()
    {
        // Formatting attachment data into VkAttachmentDescription structs

        std::vector<VkAttachmentDescription> attachmentDescriptions = {};
        const uint32_t                       attachmentCount        = mAttachmentCountColor + mAttachmentCountDepth;
        attachmentDescriptions.resize(attachmentCount);
        ManagedImage* attachments[] = {&mPositionAttachment, &mNormalAttachment, &mAlbedoAttachment, &mMotionAttachment, &mMeshIdAttachment, &mDepthAttachment};

        for(uint32_t i = 0; i < attachmentCount; i++)
        {
            attachmentDescriptions[i].samples        = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
            attachmentDescriptions[i].loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescriptions[i].storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescriptions[i].stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[i].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[i].initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescriptions[i].finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
            attachmentDescriptions[i].format         = attachments[i]->GetFormat();
        }

        // The depth attachment needs a different layout
        attachmentDescriptions[mAttachmentCountColor].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentDescriptions[mAttachmentCountColor].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Preparing attachment reference structs
        std::vector<VkAttachmentReference> attachmentReferences_Color = {};
        attachmentReferences_Color.resize(mAttachmentCountColor);
        for(uint32_t i = 0; i < mAttachmentCountColor; i++)
        {
            // Assign incremental ids
            attachmentReferences_Color[i] = VkAttachmentReference{i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }

        // the depth attachment gets the final id (one higher than the highest color attachment id)
        VkAttachmentReference attachmentReference_Depth = {mAttachmentCountColor, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = mAttachmentCountColor;
        subpass.pColorAttachments       = attachmentReferences_Color.data();
        subpass.pDepthStencilAttachment = &attachmentReference_Depth;


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
        renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachmentCount);
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;

        AssertVkResult(vkCreateRenderPass(mContext->Device, &renderPassInfo, nullptr, &mRenderpass));

        std::vector<VkImageView> attachmentViews = {mPositionAttachment.GetImageView(), mNormalAttachment.GetImageView(), mAlbedoAttachment.GetImageView(),
                                                    mMotionAttachment.GetImageView(),   mMeshIdAttachment.GetImageView(), mDepthAttachment.GetImageView()};

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = static_cast<uint32_t>(attachmentCount);
        fbufCreateInfo.width                   = mContext->Swapchain.extent.width;
        fbufCreateInfo.height                  = mContext->Swapchain.extent.height;
        fbufCreateInfo.layers                  = 1;
        AssertVkResult(vkCreateFramebuffer(mContext->Device, &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void GBufferStage::Destroy()
    {
        VkDevice device = mContext->Device;
        mPositionAttachment.Destroy();
        mNormalAttachment.Destroy();
        mAlbedoAttachment.Destroy();
        mMotionAttachment.Destroy();
        mMeshIdAttachment.Destroy();
        mDepthAttachment.Destroy();
        if(mFrameBuffer)
        {
            vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }
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
        if(mRenderpass)
        {
            vkDestroyRenderPass(device, mRenderpass, nullptr);
            mRenderpass = nullptr;
        }
        if(mPipelineCache)
        {
            vkDestroyPipelineCache(device, mPipelineCache, nullptr);
            mPipelineCache = nullptr;
        }
        mDescriptorSet.Cleanup();
    }

    void GBufferStage::SetupDescriptors()
    {
        mDescriptorSet.SetDescriptorInfoAt(0, mScene->GetMaterialUboArrayDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(1, mScene->GetTextureDescriptorInfo());
        mDescriptorSet.SetDescriptorInfoAt(2, mScene->GetTransformStateDescriptorInfo());

        //if(mScene->GetCameras().size() != 0)
        //{

        //    mDescriptorSet.SetDescriptorInfoAt(2, [0] -> GetUboDescriptorInfo());  // Assumes only one camera & always the first
        //}
        //else
        //{
        //    auto camera = new Camera(mScene);
        //}

        uint32_t              numSets             = 1;
        VkDescriptorSetLayout descriptorSetLayout = mDescriptorSet.Create(mContext, numSets);

        std::vector<VkPushConstantRange> pushConstantRanges(
            {{.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(MeshInstance::PushConstant)}});

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
        // Clear values for all attachments written in the fragment shader
        std::array<VkClearValue, 6> clearValues;
        clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[2].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[3].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[4].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[5].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mContext->Swapchain.extent;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues      = clearValues.data();

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
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, descriptorsets.size(), descriptorsets.data(), 0, nullptr);
        SceneDrawInfo drawInfo{.CmdBuffer = commandBuffer, .PipelineLayout = mPipelineLayout};
        mScene->Draw(drawInfo);  // TODO: does pipeline has to be passed? Technically a scene could build pipelines themselves.

        vkCmdEndRenderPass(commandBuffer);
    }

    void GBufferStage::PreparePipeline()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        AssertVkResult(vkCreatePipelineCache(mContext->Device, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = false,
        };

        VkPipelineRasterizationStateCreateInfo rasterizationState = {
            .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode    = VK_CULL_MODE_NONE,
            .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth   = 1.0f,
        };


        // Blend attachment states required for all color attachments
        // This is important, as color write mask will otherwise be 0x0 and you
        // won't see anything rendered to the attachment
        std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {};
        blendAttachmentStates.resize(mAttachmentCountColor);
        for(int i = 0; i < mAttachmentCountColor; i++)
        {
            blendAttachmentStates[i].blendEnable    = false;
            blendAttachmentStates[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }

        VkPipelineColorBlendStateCreateInfo colorBlendState = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size()),
            .pAttachments    = blendAttachmentStates.data(),
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilState = {
            .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable  = false,
            .depthWriteEnable = true,
            .depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL,
        };

        VkPipelineViewportStateCreateInfo viewportState = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = nullptr,
            .scissorCount  = 1,
            .pScissors     = nullptr,
        };


        VkPipelineMultisampleStateCreateInfo multisampleState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

        std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};


        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                                   .dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size()),
                                                                   .pDynamicStates    = dynamicStateEnables.data()};

        VkPipelineVertexInputStateCreateInfo vertexInputState = {};

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        auto vertShaderModule      = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/gbuffer_stage.vert.spv");
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        auto fragShaderModule      = ShaderModule(mContext, "../hsk_rt_rpf/src/shaders/gbuffer_stage.frag.spv");
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName  = "main";

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

        // vertex layout
        VertexInputStateBuilder vertexInputStateBuilder;
        vertexInputStateBuilder.AddVertexComponentBinding(VertexComponent::Position);
        vertexInputStateBuilder.AddVertexComponentBinding(VertexComponent::Normal);
        vertexInputStateBuilder.AddVertexComponentBinding(VertexComponent::Tangent);
        vertexInputStateBuilder.AddVertexComponentBinding(VertexComponent::Uv);
        vertexInputStateBuilder.AddVertexComponentBinding(VertexComponent::MaterialIndex);
        vertexInputStateBuilder.Build();

        VkGraphicsPipelineCreateInfo pipelineCI = {};
        pipelineCI.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.layout                       = mPipelineLayout;
        pipelineCI.renderPass                   = mRenderpass;
        pipelineCI.pInputAssemblyState          = &inputAssemblyState;
        pipelineCI.pRasterizationState          = &rasterizationState;
        pipelineCI.pColorBlendState             = &colorBlendState;
        pipelineCI.pMultisampleState            = &multisampleState;
        pipelineCI.pViewportState               = &viewportState;
        pipelineCI.pDepthStencilState           = &depthStencilState;
        pipelineCI.pDynamicState                = &dynamicStateCreateInfo;
        pipelineCI.stageCount                   = shaderStages.size();
        pipelineCI.pStages                      = shaderStages.data();
        pipelineCI.pVertexInputState            = &vertexInputStateBuilder.InputStateCI;

        AssertVkResult(vkCreateGraphicsPipelines(mContext->Device, mPipelineCache, 1, &pipelineCI, nullptr, &mPipeline));
    }
}  // namespace hsk