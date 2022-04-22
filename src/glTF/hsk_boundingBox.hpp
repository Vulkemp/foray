#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>

namespace hsk {
    struct BoundingBox
    {
      public:
        bool Valid() const;

        glm::vec3 min;
        glm::vec3 max;
        bool      valid = false;
        BoundingBox();
        BoundingBox(glm::vec3 min, glm::vec3 max);
        BoundingBox getAABB(glm::mat4 m);

    };


}  // namespace hsk