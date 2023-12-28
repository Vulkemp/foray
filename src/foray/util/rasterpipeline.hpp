#pragma once
#include "../basics.hpp"
#include "../core/core_declares.hpp"
#include "../core/managedresource.hpp"
#include "../scene/geo.hpp"
#include "../stages/stages_declares.hpp"
#include "../vulkan.hpp"
#include "renderattachments.hpp"

namespace foray::util {
    class RasterPipeline : public core::VulkanResource<vk::ObjectType::VK_OBJECT_TYPE_PIPELINE>
    {
      public:
        enum class BuiltinDepthInit
        {
            /// @brief No depth test
            Disabled,
            /// @brief 0: Near 1: Far
            /// @remark Be sure to clear the depth image with 1.f
            Normal,
            /// @brief 1: Near 0: Far
            /// @remark Be sure to clear the depth image with 0.f
            Inverted
        };

        class Builder
        {
          public:
            Builder& Default_SceneDrawing(VkPipelineShaderStageCreateInfo&& vertex,
                                          VkPipelineShaderStageCreateInfo&& fragment,
                                          RenderAttachments*                attachments,
                                          VkExtent2D                        extent,
                                          VkPipelineLayout                  pipelineLayout,
                                          BuiltinDepthInit                  depth = BuiltinDepthInit::Normal);
            Builder& Default_PostProcess(VkPipelineShaderStageCreateInfo&& vertex,
                                         VkPipelineShaderStageCreateInfo&& fragment,
                                         RenderAttachments*                attachments,
                                         VkExtent2D                        extent,
                                         VkPipelineLayout                  pipelineLayout);

            Builder& InitDepthStateCi(BuiltinDepthInit depthInit);
            Builder& SetAttachmentBlends(RenderAttachments* renderattachments);
            FORAY_PROPERTY_R(ShaderStages)
            FORAY_PROPERTY_R(VertexInputStateCi)
            FORAY_PROPERTY_V(PrimitiveTopology)
            FORAY_PROPERTY_V(PolygonMode)
            FORAY_PROPERTY_V(CullModeFlags)
            FORAY_PROPERTY_V(FrontFace)
            FORAY_PROPERTY_R(DepthStateCi)
            FORAY_PROPERTY_R(AttachmentBlends)
            FORAY_PROPERTY_R(DynamicStates)
            FORAY_PROPERTY_V(RenderAttachments)
            FORAY_PROPERTY_V(PipelineLayout)
            FORAY_PROPERTY_V(Extent)
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
            std::vector<VkDynamicState>                      mDynamicStates;
            RenderAttachments*                                mRenderAttachments;
            VkPipelineLayout                                 mPipelineLayout = nullptr;
            VkExtent2D                                       mExtent{};
            VkPipelineCache                                  mPipelineCache = nullptr;

          private:
            scene::VertexInputStateBuilder mVertexInputStateBuilder;
        };

        RasterPipeline(core::Context* context, const Builder& builder);
        virtual ~RasterPipeline();

        void CmdBindPipeline(VkCommandBuffer cmdBuffer);

        FORAY_GETTER_V(Pipeline)
        FORAY_GETTER_V(Extent)

      protected:
        core::Context* mContext  = nullptr;
        VkPipeline     mPipeline = nullptr;
        VkExtent2D     mExtent   = {};
    };

}  // namespace foray::util
