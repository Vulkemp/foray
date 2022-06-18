#pragma once
#include "../hsk_component.hpp"
#include "../hsk_scenegraph_declares.hpp"
#include "../../hsk_glm.hpp"

namespace hsk {
    class MeshInstance : public NodeComponent, public Component::BeforeDrawCallback, public Component::DrawCallback
    {
      public:
        inline virtual ~MeshInstance() {}

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;
        virtual void Draw(SceneDrawInfo& drawInfo) override;

        HSK_PROPERTY_ALL(InstanceIndex)
        HSK_PROPERTY_ALL(Mesh)
      protected:
        int32_t mInstanceIndex = 0;
        Mesh*   mMesh          = nullptr;
    };
}  // namespace hsk