#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>

namespace hsk {
    struct BoundingBox
    {
      public:
        BoundingBox();
        BoundingBox(glm::vec3 min, glm::vec3 max);
        BoundingBox getAABB(glm::mat4 m);

        HSK_PROPERTY_ALL(Min)
        HSK_PROPERTY_ALL(Max)
        HSK_PROPERTY_ALL(Valid)

      protected:
        glm::vec3 mMin   = {};
        glm::vec3 mMax   = {};
        bool      mValid = false;
    };


}  // namespace hsk