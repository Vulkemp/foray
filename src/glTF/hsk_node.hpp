#pragma once
#include "hsk_boundingBox.hpp"
#include "hsk_glTF_declares.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace hsk {
    class Node
    {
      public:
        Node*                 parent;
        uint32_t              index;
        std::vector<Node*>    children;
        glm::mat4             matrix;
        std::string           name;
        std::unique_ptr<Mesh> mesh;
        Skin*                 skin;
        int32_t               skinIndex = -1;
        glm::vec3             translation{};
        glm::vec3             scale{1.0f};
        glm::quat             rotation{};
        BoundingBox           bvh;
        BoundingBox           aabb;

        glm::mat4 localMatrix();
        glm::mat4 getMatrix();
        void      update();
        ~Node();
    };

}  // namespace hsk