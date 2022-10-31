#pragma once
#include "../core/foray_samplercollection.hpp"
#include "../foray_glm.hpp"
#include "foray_computestage.hpp"
#include <array>

namespace foray::stages {
    class ComparerStage : public ComputeStage
    {
      public:
        inline static constexpr std::string_view OutputName = "Comparer.Out";

        virtual void Init(core::Context* context) override;

        virtual void SetInput(uint32_t index, core::ManagedImage* image, uint32_t channelCount, glm::vec4 scale = glm::vec4(1.f));

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void HandleEvent(const osi::Event* event);
        virtual void OnResized(const VkExtent2D& extent) override;

      protected:
        virtual void ApiInitShader() override;
        virtual void ApiCreateDescriptorSetLayout() override;
        virtual void ApiCreatePipelineLayout() override;

        virtual void UpdateDescriptorSet();
        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;


        std::array<core::ManagedImage*, 2> mInputs;

        std::array<core::CombinedImageSampler, 2> mInputsSampled;

        core::ManagedImage  mMissingInput;
        core::ManagedImage  mOutput;
        core::ManagedBuffer mPipetteBuffer;
        void*               mPipetteMap = nullptr;

        glm::vec4 mPipetteValue;

        // layout (set = 0, binding = 0) uniform sampler2D Inputs[2];
        // layout (rgba32f, set = 0, binding = 2) uniform restrict writeonly image2D Output;
        // layout (std430, set = 0, binding = 3) restrict writeonly buffer VALUEBUFFER {
        //     vec4 ValueAt;
        // };
        // layout (push_constant) uniform PushConstant_T
        // {
        //     vec4 Scale[2];
        //     ivec2 MousePos;
        //     uint  Channels[2];
        //     float    Mix;
        // } PushConstant;

        struct PushConstant
        {
            glm::vec4  Scale[2];
            glm::ivec2 MousePos;
            uint32_t   Channels[2];
            fp32_t     Mix = 1.f;
            uint32_t   InputCount;
        } mPushC;
    };
}  // namespace foray::stages
