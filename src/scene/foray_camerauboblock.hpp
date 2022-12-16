#pragma once
#include "../foray_glm.hpp"

namespace foray::scene {
    /// @brief Uniform buffer object layout for camera matrices
    struct CameraUboBlock
    {
        glm::mat4 ProjectionMatrix             = {};
        glm::mat4 ViewMatrix                   = {};
        glm::mat4 PreviousProjectionMatrix     = {};
        glm::mat4 PreviousViewMatrix           = {};
        glm::mat4 ProjectionViewMatrix         = {};
        glm::mat4 PreviousProjectionViewMatrix = {};
        glm::mat4 InverseViewMatrix            = {};
        glm::mat4 InverseProjectionMatrix      = {};
    };
}