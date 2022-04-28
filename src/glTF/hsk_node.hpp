#pragma once
#include "hsk_boundingBox.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_mesh.hpp"
#include "hsk_scenecomponent.hpp"
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
        HSK_PROPERTY_ALL(Transform)
        HSK_PROPERTY_ALL(Parent)
        HSK_PROPERTY_ALL(ParentIndex)
        HSK_PROPERTY_ALL(Index)
        HSK_PROPERTY_ALL(Children)
        HSK_PROPERTY_ALL(Name)
        HSK_PROPERTY_ALL(Mesh)
        HSK_PROPERTY_ALL(Skin)
        HSK_PROPERTY_ALL(SkinIndex)
        HSK_PROPERTY_ALL(Bvh)
        HSK_PROPERTY_ALL(AxisAlignedBoundingBox)

        inline explicit Node(Scene* scene) : SceneComponent(scene) {}
        glm::mat4 getMatrix();
        void      update();
        void InitFromTinyGltfNode(const tinygltf::Model& model, const tinygltf::Node& node, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
        void ResolveParent();
        ~Node();

      protected:
        Transform          mTransform  = {};
        Node*              mParent      = nullptr;
        int32_t            mParentIndex = -1;
        int32_t            mIndex       = -1;
        std::vector<Node*> mChildren    = {};
        std::string           mName      = {};
        std::unique_ptr<hsk::Mesh> mMesh      = {};
        Skin*                 mSkin      = nullptr;
        int32_t               mSkinIndex = -1;
        BoundingBox           mBvh       = {};
        BoundingBox           mAxisAlignedBoundingBox      = {};
    };

}  // namespace hsk