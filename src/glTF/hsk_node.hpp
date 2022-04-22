#pragma once
#include "hsk_boundingBox.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_mesh.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <tinygltf/tiny_gltf.h>

namespace hsk {

    struct Transform
    {
        glm::vec3 Translation = {};
        glm::quat Rotation    = {};
        glm::vec3 Scale       = glm::vec3(1.f);
        glm::mat4 Matrix      = glm::mat4(1.f);

        glm::mat4 LocalMatrix();
        void      InitFromTinyGltfNode(const tinygltf::Node& node);
    };

    class Node : public SceneComponent, public NoMoveDefaults
    {
      public:
        Transform          mTransform  = {};
        Node*              parent      = nullptr;
        int32_t            parentIndex = -1;
        int32_t            index       = -1;
        std::vector<Node*> children    = {};
        // glm::mat4             matrix;
        std::string           name      = {};
        std::unique_ptr<Mesh> mesh      = {};
        Skin*                 skin      = nullptr;
        int32_t               skinIndex = -1;
        BoundingBox           bvh       = {};
        BoundingBox           aabb      = {};

        inline explicit Node(Scene* scene) : SceneComponent(scene) {}
        glm::mat4 getMatrix();
        void      update();
        void InitFromTinyGltfNode(const tinygltf::Model& model, const tinygltf::Node& node, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
        void ResolveParent();
        ~Node();
    };

}  // namespace hsk