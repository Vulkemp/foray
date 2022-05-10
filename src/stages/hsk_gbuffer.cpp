#include "hsk_gbuffer.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    GBufferStage::GBufferStage() {}

    // Heavily inspired from Sascha Willems' "deferred" vulkan example
    void GBufferStage::Init()
    {
        InitFixedSizeComponents();
        InitResolutionDependentComponents();
    }

    void GBufferStage::InitFixedSizeComponents()
    {
        setupDescriptorPool();
        setupDescriptorSetLayout();
        preparePipeline();
    }
    void GBufferStage::InitResolutionDependentComponents()
    {
        prepareAttachments();
        prepareRenderpass();
        setupDescriptorSet();
        buildCommandBuffer();
    }
    void GBufferStage::DestroyResolutionDependentComponents() {}

    void GBufferStage::prepareAttachments() {}

    void GBufferStage::prepareRenderpass()
    {
        // Formatting attachment data into VkAttachmentDescription structs
        const uint32_t          ATTACHMENT_COUNT_COLOR                   = 5;
        const uint32_t          ATTACHMENT_COUNT_DEPTH                   = 1;
        const uint32_t          ATTACHMENT_COUNT                         = ATTACHMENT_COUNT_COLOR + ATTACHMENT_COUNT_DEPTH;
        VkAttachmentDescription attachmentDescriptions[ATTACHMENT_COUNT] = {};
        IntermediateImage*      attachments[] = {m_PositionAttachment, m_NormalAttachment, m_AlbedoAttachment, m_MotionAttachment, m_MeshIdAttachment, m_DepthAttachment};

        for(uint32_t i = 0; i < ATTACHMENT_COUNT; i++)
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
        attachmentDescriptions[ATTACHMENT_COUNT_COLOR].finalLayout =
            VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // The depth attachment needs a different layout
        attachmentDescriptions[ATTACHMENT_COUNT_COLOR].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

        // Preparing attachment reference structs
        VkAttachmentReference attachmentReferences_Color[ATTACHMENT_COUNT_COLOR] = {};
        for(uint32_t i = 0; i < ATTACHMENT_COUNT_COLOR; i++)
        {
            attachmentReferences_Color[i] = VkAttachmentReference{i, VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};  // Assign incremental ids
        }

        VkAttachmentReference attachmentReference_Depth = {
            ATTACHMENT_COUNT_COLOR,
            VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};  // the depth attachment gets the final id (one higher than the highest color attachment id)

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = ATTACHMENT_COUNT_COLOR;
        subpass.pColorAttachments       = attachmentReferences_Color;
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
        renderPassInfo.pAttachments           = attachmentDescriptions;
        renderPassInfo.attachmentCount        = static_cast<uint32_t>(ATTACHMENT_COUNT);
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;

        HSK_ASSERT_VKRESULT(vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderpass));

        VkImageView attachmentViews[ATTACHMENT_COUNT] = {m_PositionAttachment->GetImageView(), m_NormalAttachment->GetImageView(), m_AlbedoAttachment->GetImageView(),
                                                         m_MotionAttachment->GetImageView(),   m_MeshIdAttachment->GetImageView(), m_DepthAttachment->GetImageView()};

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews;
        fbufCreateInfo.attachmentCount         = static_cast<uint32_t>(ATTACHMENT_COUNT);
        fbufCreateInfo.width                   = mRenderResolution.width;
        fbufCreateInfo.height                  = mRenderResolution.height;
        fbufCreateInfo.layers                  = 1;
        HSK_ASSERT_VKRESULT(vkCreateFramebuffer(mDevice, &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void GBufferStage::Destroy()
    {
        vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
        vkDestroyFramebuffer(mDevice, mFrameBuffer, nullptr);
        vkDestroyPipeline(mDevice, mPipeline, nullptr);
        vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
        vkDestroyRenderPass(mDevice, mRenderpass, nullptr);
        vkDestroyPipelineCache(mDevice, mPipelineCache, nullptr);
    }


    void GBufferStage::setupDescriptorPool()
    {
        // type;
        // descriptorCount;
        std::vector<VkDescriptorPoolSize> poolSizes =
            std::initializer_list<VkDescriptorPoolSize>{VkDescriptorPoolSize{VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8},
                                                        VkDescriptorPoolSize{VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}};
                                                        
        RasterizedRenderStage::InitDescriptorPool(poolSizes, 3);
    }
    void GBufferStage::setupDescriptorSetLayout()
    {
        // TODO
        // VkPipelineLayoutCreateInfo         pPipelineLayoutCreateInfoOffscreen = vks::initializers::pipelineLayoutCreateInfo(gltfDescriptorSetLayouts.data(), 2);
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        HSK_ASSERT_VKRESULT(vkCreatePipelineLayout(mDevice, &pipelineLayoutCI, nullptr, &mPipelineLayout));
    }

    void GBufferStage::setupDescriptorSet()
    {
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;

        // Model
        // use descriptor set layout delivered by gltf
        // VkDescriptorSetAllocateInfo allocInfoOffscreen = vks::initializers::descriptorSetAllocateInfo(mDescriptorPool, &vkglTF::descriptorSetLayoutUbo, 1);
        // HSK_ASSERT_VKRESULT(vkAllocateDescriptorSets(mDevice, &allocInfoOffscreen, &mDescriptorSetScene));
        // writeDescriptorSets = {// Binding 0: Vertex shader uniform buffer
        //                        m_rtFilterDemo->m_UBO_SceneInfo->writeDescriptorSet(mDescriptorSetScene, 0)};
        // vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

    void GBufferStage::RecordFrame(FrameRenderInfo& renderInfo)
    {
        // Clear values for all attachments written in the fragment shader
        std::array<VkClearValue, 6> clearValues;
        clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[1].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[3].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[4].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[5].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderpass;
        renderPassBeginInfo.framebuffer       = mFrameBuffer;
        renderPassBeginInfo.renderArea.extent = mRenderResolution;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues      = clearValues.data();

        vkCmdBeginRenderPass(renderInfo.GetCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // = vks::initializers::viewport((float)mRenderResolution.width, (float)mRenderResolution.height, 0.0f, 1.0f);
        VkViewport viewport{0.f, 0.f, (float)mRenderResolution.width, (float)mRenderResolution.height, 0.0f, 1.0f};
        vkCmdSetViewport(renderInfo.GetCommandBuffer(), 0, 1, &viewport);

        VkRect2D scissor{VkOffset2D{}, VkExtent2D{mRenderResolution.width, mRenderResolution.height}};
        vkCmdSetScissor(renderInfo.GetCommandBuffer(), 0, 1, &scissor);

        vkCmdBindPipeline(renderInfo.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        // Instanced object
        vkCmdBindDescriptorSets(renderInfo.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSetScene, 0, nullptr);
        m_Scene->draw(renderInfo.GetCommandBuffer(), vkglTF::RenderFlags::BindImages, mPipelineLayout, 1);  // vkglTF::RenderFlags::BindImages

        vkCmdEndRenderPass(renderInfo.GetCommandBuffer());

        HSK_ASSERT_VKRESULT(vkEndCommandBuffer(renderInfo.GetCommandBuffer()));
    }

    void GBufferStage::preparePipeline()
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        HSK_ASSERT_VKRESULT(vkCreatePipelineCache(mDevice, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState =
            vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        VkPipelineColorBlendAttachmentState   blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo   colorBlendState      = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState    = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo     viewportState        = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo  multisampleState     = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState>           dynamicStateEnables  = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo      dynamicState         = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        VkPipelineVertexInputStateCreateInfo  vertexInputState     = vks::initializers::pipelineVertexInputStateCreateInfo();
        VkPipelineShaderStageCreateInfo       shaderStages[2]{m_rtFilterDemo->LoadShader("prepass/rasterprepass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
                                                        m_rtFilterDemo->LoadShader("prepass/rasterprepass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

        VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(mPipelineLayout, m_renderpass);
        pipelineCI.pInputAssemblyState          = &inputAssemblyState;
        pipelineCI.pRasterizationState          = &rasterizationState;
        pipelineCI.pColorBlendState             = &colorBlendState;
        pipelineCI.pMultisampleState            = &multisampleState;
        pipelineCI.pViewportState               = &viewportState;
        pipelineCI.pDepthStencilState           = &depthStencilState;
        pipelineCI.pDynamicState                = &dynamicState;
        pipelineCI.stageCount                   = 2;
        pipelineCI.pStages                      = shaderStages;

        pipelineCI.pVertexInputState =
            vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color,
                                                         vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Tangent, vkglTF::VertexComponent::MeshId});
        rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

        // Blend attachment states required for all color attachments
        // This is important, as color write mask will otherwise be 0x0 and you
        // won't see anything rendered to the attachment
        std::array<VkPipelineColorBlendAttachmentState, 5> blendAttachmentStates = {vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                                                                                    vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                                                                                    vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                                                                                    vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                                                                                    vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)};

        colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
        colorBlendState.pAttachments    = blendAttachmentStates.data();

        HSK_ASSERT_VKRESULT(vkCreateGraphicsPipelines(mDevice, mPipelineCache, 1, &pipelineCI, nullptr, &mPipeline));
    }
}  // namespace hsk