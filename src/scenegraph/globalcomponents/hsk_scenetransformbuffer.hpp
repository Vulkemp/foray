#pragma once
#include "../../memory/hsk_managedvectorbuffer.hpp"
#include "../hsk_component.hpp"
#include <glm/glm.hpp>

namespace hsk {

    struct alignas(16) NModelTransformState
    {
        glm::mat4 ModelMatrix         = glm::mat4(1.f);
        glm::mat4 PreviousModelMatrix = glm::mat4(1.f);
    };

    /// @brief Stores all current and previous' frame model matrices for use by the GPU
    class SceneTransformBuffer : public GlobalComponent, public Component::BeforeDrawCallback
    {
      public:
        explicit SceneTransformBuffer(const VkContext* context);

        void Resize(size_t size);

        void UpdateSceneTransform(int32_t meshInstanceIndex, const glm::mat4& modelMatrix);

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;

        inline virtual std::vector<NModelTransformState>& GetVector() { return mBuffer.GetVector(); }

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MakeDescriptorInfo();

      protected:
        std::vector<bool>                         mTouchedTransforms;
        ManagedVectorBuffer<NModelTransformState> mBuffer;
    };
}  // namespace hsk