#pragma once
#include "../core/foray_samplercollection.hpp"
#include "../foray_glm.hpp"
#include "foray_computestage.hpp"
#include <array>

namespace foray::stages {
    class ComparerStage : public RenderStage
    {
      public:
        inline static constexpr std::string_view OutputName = "Comparer.Out";

        virtual void Init(core::Context* context);

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        virtual void HandleEvent(const osi::Event* event);
        virtual void OnResized(const VkExtent2D& extent) override;

        virtual void Destroy() override;

        enum class EInputType
        {
            Float,
            Int,
            Uint
        };

        struct Input
        {
            core::ManagedImage* Image;
            uint32_t            ChannelCount;
            glm::vec4           Scale;
            VkImageAspectFlags  Aspect;
            EInputType          Type;
        };

        struct PipetteValue
        {
            glm::vec4  Value;
            glm::vec2  UvPos;
            glm::ivec2 TexelPos;
        };

        virtual void SetInput(uint32_t index, const Input& input);

        FORAY_PROPERTY_ALL(MixValue)
        FORAY_PROPERTY_CGET(PipetteValue)

      protected:
        struct SubStage
        {
            uint32_t                   Index;
            Input         Input;
            core::CombinedImageSampler InputSampled;
            core::ShaderModule*        Shader;
            core::DescriptorSet        DescriptorSet;
            util::PipelineLayout       PipelineLayout;
            VkPipeline                 Pipeline = nullptr;
        };

        void CreateSubStage(SubStage& substage);
        void DispatchSubStage(SubStage& substage, VkCommandBuffer buffer, base::FrameRenderInfo& renderInfo);
        void DestroySubStage(SubStage& substage, bool final);

        struct PushConstant
        {
            glm::vec4  Scale;
            glm::ivec2 MousePos;
            uint32_t   Channels;
            fp32_t     Mix;
            uint32_t   WriteOffset;
            VkBool32   WriteLeft;
        };

        virtual void CreateFixedSizeComponents() override;
        virtual void DestroyFixedComponents() override;
        virtual void CreateResolutionDependentComponents() override;
        virtual void DestroyResolutionDependentComponents() override;

        std::array<SubStage, 2> mSubStages;

        core::ManagedImage  mOutput;
        core::ManagedBuffer mPipetteBuffer;
        void*               mPipetteMap = nullptr;

        PipetteValue mPipetteValue;

        std::array<core::ShaderModule, 3> mShaders;

        fp32_t     mMixValue = 0.5;
        glm::ivec2 mMousePos = {};
    };
}  // namespace foray::stages
