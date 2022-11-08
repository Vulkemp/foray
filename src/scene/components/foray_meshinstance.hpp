#pragma once
#include "../../foray_glm.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"

namespace foray::scene::ncomp {
    /// @brief Defines an instance of a mesh
    class MeshInstance : public NodeComponent
    {
      public:
        inline virtual ~MeshInstance() {}

        FORAY_PROPERTY_ALL(InstanceIndex)
        FORAY_PROPERTY_ALL(Mesh)
      protected:
        int32_t   mInstanceIndex       = 0;
        Mesh*     mMesh                = nullptr;
    };
}  // namespace foray