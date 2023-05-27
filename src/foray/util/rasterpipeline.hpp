#pragma once
#include "../basics.hpp"
#include "../core/core_declares.hpp"
#include "../scene/geo.hpp"
#include "../stages/stages_declares.hpp"
#include "../core/managedresource.hpp"
#include "../vulkan.hpp"
#include "renderpass.hpp"

namespace foray::util {
    class RasterPipeline : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_PIPELINE>
    {
      public:
        enum class BuiltinDepthInit
        {
            Disabled,
            Normal,
            Inverted
        };

        class Builder
        {
          public:
            Builder& InitDepthStateCi(BuiltinDepthInit depthInit);
            Builder& InitDefaultAttachmentBlendStates(bool hasDepth);
            FORAY_PROPERTY_R(ShaderStages)
            FORAY_PROPERTY_R(VertexInputStateCi)
            FORAY_PROPERTY_V(PrimitiveTopology)
            FORAY_PROPERTY_V(PolygonMode)
            FORAY_PROPERTY_V(CullModeFlags)
            FORAY_PROPERTY_V(FrontFace)
            FORAY_PROPERTY_R(DepthStateCi)
            FORAY_PROPERTY_R(AttachmentBlends)
            FORAY_PROPERTY_V(PipelineLayout)
            FORAY_PROPERTY_V(Renderpass)
            FORAY_PROPERTY_V(PipelineCache)
          protected:
            std::vector<VkPipelineShaderStageCreateInfo>     mShaderStages;
            VkPipelineVertexInputStateCreateInfo             mVertexInputStateCi{};
            VkPrimitiveTopology                              mPrimitiveTopology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPolygonMode                                    mPolygonMode       = VkPolygonMode::VK_POLYGON_MODE_FILL;
            VkCullModeFlags                                  mCullModeFlags     = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
            VkFrontFace                                      mFrontFace         = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
            VkPipelineDepthStencilStateCreateInfo            mDepthStateCi{};
            std::vector<VkPipelineColorBlendAttachmentState> mAttachmentBlends;
            VkPipelineLayout                                 mPipelineLayout;
            Renderpass*                                      mRenderpass    = nullptr;
            VkPipelineCache                                  mPipelineCache = nullptr;
        };

        RasterPipeline(core::Context* context, const Builder& builder, stages::RenderDomain* domain);
        virtual ~RasterPipeline();

        void CmdBindPipeline(VkCommandBuffer cmdBuffer);

        FORAY_GETTER_V(Pipeline)
        FORAY_GETTER_V(Extent)
        FORAY_GETTER_V(Renderpass)

      protected:
        core::Context* mContext    = nullptr;
        VkPipeline     mPipeline   = nullptr;
        Renderpass*    mRenderpass = nullptr;
        VkExtent2D     mExtent = {};
    };

}  // namespace foray::util
