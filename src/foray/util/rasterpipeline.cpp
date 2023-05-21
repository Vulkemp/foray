#include "rasterpipeline.hpp"
#include "../stages/renderdomain.hpp"

namespace foray::util {
    RasterPipeline::RasterPipeline(core::Context* context, const Builder& builder, stages::RenderDomain* domain)
        : mContext(context), mRenderpass(builder.GetRenderpass()), mDomain(domain)
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

        VkExtent2D extent = mDomain->GetExtent();

        VkViewport viewport{
            .x = 0,
            .y = 0,
            .width = (fp32_t)extent.width,
            .height = (fp32_t)extent.height,
            .minDepth = 0,
            .maxDepth = 1
        };
        VkRect2D scissors{
            .offset = {0u, 0u},
            .extent = extent
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
            .rasterizerDiscardEnable = VK_TRUE,
            .polygonMode = builder.GetPolygonMode(),
            .cullMode = builder.GetCullModeFlags(),
            .frontFace = builder.GetFrontFace(),
            .depthBiasEnable = VK_FALSE,
            .lineWidth = builder.GetPolygonMode() == VkPolygonMode::VK_POLYGON_MODE_LINE ? 1.f : 0.f
        };

        VkPipelineMultisampleStateCreateInfo multisampleCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .alphaToCoverageEnable = VK_FALSE
        };

        VkPipelineColorBlendStateCreateInfo colorBlendCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = (uint32_t)builder.GetAttachmentBlends().size(),
            .pAttachments = builder.GetAttachmentBlends().data(),
            .blendConstants = {1.f, 1.f, 1.f, 1.f}
        };

        VkGraphicsPipelineCreateInfo pipelineCi{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = (uint32_t)builder.GetShaderStages().size(),
            .pStages = builder.GetShaderStages().data(),
            .pVertexInputState = &builder.GetVertexInputStateBuilder(),
            .pInputAssemblyState = &inputAssemblyCi,
            .pTessellationState = &tesselationCi,
            .pViewportState = &viewportCi,
            .pRasterizationState = &rasterCi,
            .pMultisampleState = &multisampleCi,
            .pDepthStencilState = &builder.GetDepthStateCi(),
            .pColorBlendState = ,
            .pDynamicState = ,
            .layout = ,
            .renderPass = ,
            .subpass = ,
            .basePipelineHandle = mPipeline,
            .basePipelineIndex = 0u,
        };

        // clang-format on
    }

    RasterPipeline::~RasterPipeline() {}


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
                mDepthStateCi = VkPipelineDepthStencilStateCreateInfo{
                    .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .depthTestEnable = VK_TRUE,
                    .depthWriteEnable = VK_TRUE,
                    .depthCompareOp = VkCompareOp::VK_COMPARE_OP_GREATER,
                    .depthBoundsTestEnable = VK_FALSE,
                    .stencilTestEnable = VK_FALSE
                };
                break;
            // case BuiltinDepthInit::Inverted:
            //     mDepthStateCi = VkPipelineDepthStencilStateCreateInfo{
            //         .sType = ,
            //         .pNext = ,
            //         .flags = ,
            //         .depthTestEnable = ,
            //         .depthWriteEnable = ,
            //         .depthCompareOp = ,
            //         .depthBoundsTestEnable = ,
            //         .stencilTestEnable = ,
            //         .front = ,
            //         .back = ,
            //         .minDepthBounds = ,
            //         .maxDepthBounds = ,
            //     };
            //     break;
        }
        return *this;
    }

    RasterPipeline::Builder& RasterPipeline::Builder::InitDefaultAttachmentBlendStates(bool hasDepth)
    {
        return *this;
    }

}  // namespace foray::util
