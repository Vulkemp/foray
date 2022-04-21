#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>

namespace hsk{
        class BoundingBox
    {
        public:
        glm::vec3 min;
        glm::vec3 max;
        bool      valid = false;
        BoundingBox();
        BoundingBox(glm::vec3 min, glm::vec3 max);
        BoundingBox getAABB(glm::mat4 m);
    };


}