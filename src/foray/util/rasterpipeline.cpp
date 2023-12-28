#include "rasterpipeline.hpp"
#include "../core/context.hpp"
#include "../stages/renderdomain.hpp"

namespace foray::util {
    RasterPipeline::RasterPipeline(core::Context* context, const Builder& builder) : mContext(context), mExtent(builder.GetExtent())
    {
        // clang-format off
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = builder.GetPrimitiveTopology()
        };

        VkPipelineTessellationStateCreateInfo tesselationCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .patchControlPoints = 1u
        };

        VkViewport viewport{
            .x = 0,
            .y = 0,
            .width = (fp32_t)mExtent.width,
            .height = (fp32_t)mExtent.height,
            .minDepth = 0,
            .maxDepth = 1
        };
        VkRect2D scissors{
            .offset = {0u, 0u},
            .extent = mExtent
        };

        VkPipelineViewportStateCreateInfo viewportCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1u,
            .pViewports = &viewport,
            .scissorCount = 1u,
            .pScissors = &scissors,
        };

        VkPipelineRasterizationStateCreateInfo rasterCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = builder.GetPolygonMode(),
            .cullMode = builder.GetCullModeFlags(),
            .frontFace = builder.GetFrontFace(),
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.f
        };

        VkPipelineMultisampleStateCreateInfo multisampleCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = vk::SampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .alphaToCoverageEnable = VK_FALSE
        };

        VkPipelineColorBlendStateCreateInfo colorBlendCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VkLogicOp::VK_LOGIC_OP_COPY,
            .attachmentCount = (uint32_t)builder.GetAttachmentBlends().size(),
            .pAttachments = builder.GetAttachmentBlends().data(),
            .blendConstants = {0.f, 0.f, 0.f, 0.f}
        };

        VkPipelineDynamicStateCreateInfo dynamicCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = (uint32_t)builder.GetDynamicStates().size(),
            .pDynamicStates = builder.GetDynamicStates().data(),
        };

        VkPipelineRenderingCreateInfo renderingCi = builder.GetRenderAttachments()->MakePipelineRenderingCi();

        VkGraphicsPipelineCreateInfo pipelineCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingCi,
            .flags = 0,
            .stageCount = (uint32_t)builder.GetShaderStages().size(),
            .pStages = builder.GetShaderStages().data(),
            .pVertexInputState = &builder.GetVertexInputStateCi(),
            .pInputAssemblyState = &inputAssemblyCi,
            .pTessellationState = &tesselationCi,
            .pViewportState = &viewportCi,
            .pRasterizationState = &rasterCi,
            .pMultisampleState = &multisampleCi,
            .pDepthStencilState = &builder.GetDepthStateCi(),
            .pColorBlendState = &colorBlendCi,
            .pDynamicState = &dynamicCi,
            .layout = builder.GetPipelineLayout(),
            .renderPass = nullptr,
            .subpass = 0u,
            .basePipelineHandle = mPipeline,
            .basePipelineIndex = 0u,
        };
        // clang-format on

        AssertVkResult(vkCreateGraphicsPipelines(mContext->VkDevice(), builder.GetPipelineCache(), 1u, &pipelineCi, nullptr, &mPipeline));
    }

    RasterPipeline::~RasterPipeline()
    {
        vkDestroyPipeline(mContext->VkDevice(), mPipeline, nullptr);
    }

    void RasterPipeline::CmdBindPipeline(VkCommandBuffer cmdBuffer)
    {
        vkCmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    }


    RasterPipeline::Builder& RasterPipeline::Builder::Default_SceneDrawing(VkPipelineShaderStageCreateInfo&& vertex,
                                                                           VkPipelineShaderStageCreateInfo&& fragment,
                                                                           RenderAttachments*                attachments,
                                                                           VkExtent2D                        extent,
                                                                           VkPipelineLayout                  pipelineLayout,
                                                                           BuiltinDepthInit                  depth)
    {
        mRenderAttachments = attachments;
        mShaderStages.reserve(2);
        mShaderStages.emplace_back(vertex);
        mShaderStages.emplace_back(fragment);
        mVertexInputStateBuilder = scene::VertexInputStateBuilder();
        mVertexInputStateBuilder.AddVertexComponentBinding(scene::EVertexComponent::Position)
            .AddVertexComponentBinding(scene::EVertexComponent::Normal)
            .AddVertexComponentBinding(scene::EVertexComponent::Tangent)
            .AddVertexComponentBinding(scene::EVertexComponent::Uv)
            .Build();
        SetVertexInputStateCi(mVertexInputStateBuilder.InputStateCI);
        InitDepthStateCi(depth);
        SetExtent(extent);
        SetAttachmentBlends(attachments);
        mDynamicStates = {VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT, VkDynamicState::VK_DYNAMIC_STATE_SCISSOR};
        SetPipelineLayout(pipelineLayout);
        return *this;
    }

    RasterPipeline::Builder& RasterPipeline::Builder::Default_PostProcess(VkPipelineShaderStageCreateInfo&& vertex,
                                                                          VkPipelineShaderStageCreateInfo&& fragment,
                                                                           RenderAttachments*                attachments,
                                                                           VkExtent2D                        extent,
                                                                          VkPipelineLayout                  pipelineLayout)
    {
        mRenderAttachments = attachments;
        mShaderStages.reserve(2);
        mShaderStages.emplace_back(vertex);
        mShaderStages.emplace_back(fragment);
        mVertexInputStateCi = VkPipelineVertexInputStateCreateInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        mCullModeFlags      = VkCullModeFlagBits::VK_CULL_MODE_NONE;
        InitDepthStateCi(BuiltinDepthInit::Disabled);
        SetExtent(extent);
        SetAttachmentBlends(attachments);
        mDynamicStates = {VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT, VkDynamicState::VK_DYNAMIC_STATE_SCISSOR};
        SetPipelineLayout(pipelineLayout);
        return *this;
    }

    RasterPipeline::Builder& RasterPipeline::Builder::InitDepthStateCi(BuiltinDepthInit depthInit)
    {
        switch(depthInit)
        {
            case BuiltinDepthInit::Disabled:
                mDepthStateCi = VkPipelineDepthStencilStateCreateInfo{
                    .sType             = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .depthTestEnable   = VK_FALSE,
                    .stencilTestEnable = VK_FALSE,
                };
                break;
            case BuiltinDepthInit::Normal:
                mDepthStateCi = VkPipelineDepthStencilStateCreateInfo{.sType                 = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                                                      .depthTestEnable       = VK_TRUE,
                                                                      .depthWriteEnable      = VK_TRUE,
                                                                      .depthCompareOp        = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL,
                                                                      .depthBoundsTestEnable = VK_FALSE,
                                                                      .stencilTestEnable     = VK_FALSE,
                                                                      .maxDepthBounds        = 1.f};
                break;
            case BuiltinDepthInit::Inverted:
                mDepthStateCi = VkPipelineDepthStencilStateCreateInfo{
                    .sType                 = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .depthTestEnable       = VK_TRUE,
                    .depthWriteEnable      = VK_TRUE,
                    .depthCompareOp        = VkCompareOp::VK_COMPARE_OP_GREATER,
                    .depthBoundsTestEnable = VK_FALSE,
                    .stencilTestEnable     = VK_FALSE,
                    .minDepthBounds        = 1.f,
                };
                break;
            default:
                FORAY_THROWFMT("Unhandled value {}", NAMEOF_ENUM(depthInit))
        }
        return *this;
    }

    RasterPipeline::Builder& RasterPipeline::Builder::SetAttachmentBlends(RenderAttachments* attachments)
    {
        uint32_t colorAttachmentCount = attachments->GetAttachments().size();
        if(colorAttachmentCount > 0)
        {
            const VkColorComponentFlags flags = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
                                                | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
            mAttachmentBlends =
                std::vector<VkPipelineColorBlendAttachmentState>(colorAttachmentCount, VkPipelineColorBlendAttachmentState{.blendEnable = VK_FALSE, .colorWriteMask = flags});
        }
        return *this;
    }

}  // namespace foray::util
