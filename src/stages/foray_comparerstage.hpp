#pragma once
#include "../core/foray_samplercollection.hpp"
#include "../foray_glm.hpp"
#include "foray_computestage.hpp"
#include <array>

namespace foray::stages {

    /// @brief Displays two images of any type next to each other, and get a "pipette" readout at the mouse location
    class ComparerStage : public RenderStage
    {
      public:
        /// @brief Image output name
        inline static constexpr std::string_view OutputName = "Comparer.Out";

        /// @brief Inits the comparer stage. SetInput() calls afterwards are required for function
        virtual void Init(core::Context* context, RenderDomain* domain, bool flipY, int32_t resizeOrder = 0);

        /// @brief Pipeline barriers and compute shader dispatches
        virtual void RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo) override;

        /// @brief If called the comparer stage will filter for MouseMoved events to update the pipette value returned
        virtual void HandleMouseMovedEvent(const osi::EventInputMouseMoved* event);

        virtual void Destroy() override;

        enum class EInputType
        {
            Float,
            Int,
            Uint
        };

        /// @brief Argument struct for setting inputs
        struct InputInfo
        {
            /// @brief Image
            core::ManagedImage* Image = nullptr;
            /// @brief Channels per pixel
            uint32_t ChannelCount = 4;
            /// @brief Scale applied to each channel before writing to output
            glm::vec4 Scale = glm::vec4(1.f);
            /// @brief Aspect required for the barrier to function
            VkImageAspectFlags Aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
            /// @brief Channel type (required to select correct shader input)
            EInputType Type = EInputType::Float;
        };

        struct PipetteValue
        {
            glm::vec4  Value;
            glm::vec2  UvPos;
            glm::ivec2 TexelPos;
        };

        /// @brief Set the input
        /// @param index 0 == left, 1 == right
        /// @param input Image Input information
        virtual void SetInput(uint32_t index, const InputInfo& input);

        /// @brief Value between 0...1 defining the split value
        FORAY_PROPERTY_V(MixValue)
        FORAY_GETTER_CR(PipetteValue)

      protected:
        struct SubStage
        {
            uint32_t                   Index;
            InputInfo                  Input;
            core::CombinedImageSampler InputSampled;
            core::ShaderModule*        Shader;
            core::DescriptorSet        DescriptorSet;
            util::PipelineLayout       PipelineLayout;
            VkPipeline                 Pipeline = nullptr;
        };

        void CreateSubStage(SubStage& substage);
        void DispatchSubStage(SubStage& substage, VkCommandBuffer buffer, base::FrameRenderInfo& renderInfo);
        void DestroySubStage(SubStage& substage, bool final);

        void CreateOutputImage();

        void LoadShaders();
        void CreatePipetteBuffer();

        virtual void OnResized(VkExtent2D extent) override;

        struct PushConstant
        {
            glm::vec4  Scale;
            glm::ivec2 MousePos;
            uint32_t   Channels;
            fp32_t     Mix;
            uint32_t   WriteOffset;
            VkBool32   WriteLeft;
        };

        std::array<SubStage, 2> mSubStages;

        core::ManagedImage  mOutput;
        core::ManagedBuffer mPipetteBuffer;
        void*               mPipetteMap = nullptr;

        PipetteValue mPipetteValue;

        std::array<core::ShaderModule, 3> mShaders;

        fp32_t     mMixValue = 0.5;
        glm::ivec2 mMousePos = {};
        bool       mFlipY    = false;

        event::Receiver<const osi::EventInputMouseMoved*> mOnMouseMoved;
    };
}  // namespace foray::stages
