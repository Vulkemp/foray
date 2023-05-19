#pragma once
#include "../../glm.hpp"
#include "../component.hpp"
#include "../scene_declares.hpp"

namespace foray::scene::ncomp {
    /// @brief Defines an instance of a mesh
    class MeshInstance : public NodeComponent
    {
      public:
        inline virtual ~MeshInstance() {}

        FORAY_PROPERTY_V(InstanceIndex)
        FORAY_PROPERTY_V(Mesh)
      protected:
        int32_t   mInstanceIndex       = 0;
        Mesh*     mMesh                = nullptr;
    };
}  // namespace foray