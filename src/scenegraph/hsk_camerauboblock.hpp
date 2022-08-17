#pragma once
#include "../hsk_glm.hpp"

namespace hsk {
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