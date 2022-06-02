#pragma once
#include "../hsk_basics.hpp"
#include "hsk_gltf_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <glm/glm.hpp>

namespace hsk {

    struct ModelTransformState
    {
        glm::mat4 ModelMatrix;
        glm::mat4 PreviousModelMatrix;
    };

    class SceneTransformState : public SceneComponent, public NoMoveDefaults
    {
        
    };
}  // namespace hsk