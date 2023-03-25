#include "foray_pipelinebuilder.hpp"
#include "../core/foray_context.hpp"

namespace foray::util {
    VkPipeline PipelineBuilder::Build()
    {
        if(mContext == nullptr)
        {
            throw Exception("Set context before building pipeline!");
        }

        if(mShaderStageCreateInfos->size() == 0)
        {
            throw Exception("Add shader stage create infos before building!");
        }

        if(mRenderPass == nullptr)
        {
            throw Exception("No renderpass set for pipeline building");
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
        inputAssemblyState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology                               = mPrimitiveTopology;
        inputAssemblyState.primitiveRestartEnable                 = VK_FALSE;

        VkViewport viewport = {};
        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = (float)mDomain->GetExtent().width;
        viewport.height     = (float)mDomain->GetExtent().height;
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;

        VkRect2D scissor = {};
        scissor.offset   = {0, 0};
        scissor.extent   = mDomain->GetExtent();

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount                     = 1;
        viewportState.pViewports                        = &viewport;
        viewportState.scissorCount                      = 1;
        viewportState.pScissors                         = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizerState = {};
        rasterizerState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerState.depthClampEnable                       = VK_FALSE;
        rasterizerState.rasterizerDiscardEnable                = VK_FALSE;
        rasterizerState.polygonMode                            = VK_POLYGON_MODE_FILL;
        rasterizerState.lineWidth                              = 1.0f;
        rasterizerState.cullMode                               = mCullMode;  //VK_CULL_MODE_BACK_BIT;
        rasterizerState.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
        rasterizerState.depthBiasEnable                        = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable                  = VK_FALSE;
        multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable       = mDepthTestEnable;
        depthStencil.depthWriteEnable      = mDepthWriteEnable;
        depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable     = VK_FALSE;
        depthStencil.minDepthBounds        = 0;
        depthStencil.maxDepthBounds        = 10000;

        std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {};
        if(mColorBlendAttachmentStates == nullptr)
        {
            blendAttachmentStates.resize(mColorAttachmentBlendCount);
            for(int32_t i = 0; i < (int32_t)mColorAttachmentBlendCount; i++)
            {
                blendAttachmentStates[i].blendEnable    = false;
                blendAttachmentStates[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            }
        }

        VkPipelineColorBlendStateCreateInfo colorBlendState = {};
        colorBlendState.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.pNext                               = 0;
        colorBlendState.flags                               = 0;
        colorBlendState.blendConstants[0]                   = 0.0f;
        colorBlendState.blendConstants[1]                   = 0.0f;
        colorBlendState.blendConstants[2]                   = 0.0f;
        colorBlendState.blendConstants[3]                   = 0.0f;
        colorBlendState.logicOpEnable                       = VK_FALSE;
        colorBlendState.logicOp                             = VK_LOGIC_OP_COPY;
        colorBlendState.attachmentCount = static_cast<uint32_t>(mColorBlendAttachmentStates != nullptr ? mColorBlendAttachmentStates->size() : blendAttachmentStates.size());
        colorBlendState.pAttachments    = mColorBlendAttachmentStates != nullptr ? mColorBlendAttachmentStates->data() : blendAttachmentStates.data();

        std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicInfo = {};
        dynamicInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicInfo.dynamicStateCount                = static_cast<uint32_t>(mDynamicStates == nullptr ? dynamicStates.size() : mDynamicStates->size());
        dynamicInfo.pDynamicStates                   = mDynamicStates == nullptr ? dynamicStates.data() : mDynamicStates->data();

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount                   = mShaderStageCreateInfos->size();
        pipelineInfo.pStages                      = mShaderStageCreateInfos->data();
        pipelineInfo.pVertexInputState            = &mVertexInputStateBuilder->InputStateCI;
        pipelineInfo.pInputAssemblyState          = &inputAssemblyState;
        pipelineInfo.pViewportState               = &viewportState;
        pipelineInfo.pRasterizationState          = &rasterizerState;
        pipelineInfo.pMultisampleState            = &multisampling;
        pipelineInfo.pDepthStencilState           = &depthStencil;
        pipelineInfo.pColorBlendState             = &colorBlendState;
        pipelineInfo.pDynamicState                = &dynamicInfo;
        pipelineInfo.layout                       = mPipelineLayout;
        pipelineInfo.renderPass                   = mRenderPass;
        pipelineInfo.subpass                      = 0;
        pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;

        VkPipeline pipeline;
        AssertVkResult(vkCreateGraphicsPipelines(mContext->Device(), mPipelineCache, 1, &pipelineInfo, nullptr, &pipeline));
        return pipeline;
    }
}  // namespace foray