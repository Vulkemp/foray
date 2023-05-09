#pragma once
#include "../core/foray_shadermodule.hpp"
#include "../foray_glm.hpp"
#include "../foray_mem.hpp"
#include "../util/foray_descriptorsetsimple.hpp"
#include "../util/foray_pipelinelayout.hpp"
#include "foray_renderstage.hpp"

namespace foray::stages {
    /// @brief Base class for compute shaders
    /// @details
    /// # Features
    ///  * Pipeline building
    ///  * Binding and Dispatch
    class ComputeStageBase : public RenderStage
    {
      public:
        /// @brief Init
        /// @details Calls ApiCreateDescriptorSet(), ApiCreatePipelineLayout(), ApiInitShader(), CreatePipeline() in this order
        ComputeStageBase(core::Context* context, RenderDomain* domain = nullptr, uint32_t resizeOrder = 0);

        /// @brief Calls ApiBeforeFrame(), binds pipeline and descriptor set, calls ApiBeforeDispatch()
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual ~ComputeStageBase();

      protected:
        Local<core::ShaderModule> mShader;

        util::DescriptorSetSimple   mDescriptorSet;
        Local<util::PipelineLayout> mPipelineLayout;

        VkPipeline mPipeline = nullptr;

        /// @brief Prepare resources used in the compute shader
        inline virtual void ApiBeforeFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo){};
        /// @brief Push constants and configure the Group size
        /// @param groupSize Compute workgroup counts
        inline virtual void ApiBeforeDispatch(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo, glm::uvec3& groupSize){};

        virtual void CreatePipeline();
    };
}  // namespace foray::stages