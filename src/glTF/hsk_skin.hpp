#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>

namespace hsk {

    class Skin
    {
      public:
        std::string            name                = {};
        Node*                  skeletonRoot        = nullptr;
        std::vector<glm::mat4> inverseBindMatrices = {};
        std::vector<Node*>     joints              = {};
    };

}  // namespace hsk