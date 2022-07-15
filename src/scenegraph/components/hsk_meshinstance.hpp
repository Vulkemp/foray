#pragma once
#include "../../hsk_glm.hpp"
#include "../hsk_component.hpp"
#include "../hsk_scenegraph_declares.hpp"

namespace hsk {
    class MeshInstance : public NodeComponent
    {
      public:
        inline virtual ~MeshInstance() {}

        HSK_PROPERTY_ALL(InstanceIndex)
        HSK_PROPERTY_ALL(Mesh)
      protected:
        int32_t   mInstanceIndex       = 0;
        Mesh*     mMesh                = nullptr;
        glm::mat4 mPreviousWorldMatrix = glm::mat4(1);
    };
}  // namespace hsk