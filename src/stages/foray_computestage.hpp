#pragma once
#include "../core/foray_shadermodule.hpp"
#include "../foray_glm.hpp"
#include "../util/foray_pipelinelayout.hpp"
#include "foray_renderstage.hpp"

namespace foray::stages {
    class ComputeStage : public RenderStage
    {
      public:
        virtual void Init(core::Context* context);

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

      protected:
        core::ShaderModule mShader;

        core::DescriptorSet  mDescriptorSet;
        util::PipelineLayout mPipelineLayout;

        VkPipeline mPipeline = nullptr;

        inline virtual void ApiInitShader(){};
        inline virtual void ApiCreateDescriptorSetLayout(){};
        inline virtual void ApiCreatePipelineLayout(){};
        inline virtual void ApiBeforeFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo){};
        inline virtual void ApiBeforeDispatch(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo, glm::uvec3& groupSize){};

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
    };
}  // namespace foray::stages