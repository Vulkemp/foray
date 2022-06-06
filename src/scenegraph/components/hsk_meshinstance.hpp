#pragma once
#include "../hsk_component.hpp"
#include <glm/glm.hpp>

namespace hsk {
    class NMeshInstance : public NodeComponent, public Component::BeforeDrawCallback, public Component::DrawCallback
    {
      public:
        inline virtual ~NMeshInstance() {}

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;
        virtual void Draw(SceneDrawInfo& drawInfo) override;

      protected:
        int32_t                 mInstanceIndex = 0;
        int32_t                 mMeshIndex     = 0;
    };
}  // namespace hsk