#include "hsk_node.hpp"
#include "hsk_mesh.hpp"
#include "hsk_scene.hpp"
#include "hsk_scenedrawinfo.hpp"

namespace hsk {
    glm::mat4 Transform::LocalMatrix() { return glm::translate(glm::mat4(1.0f), Translation) * glm::mat4(Rotation) * glm::scale(glm::mat4(1.0f), Scale) * Matrix; }

    void Transform::InitFromTinyGltfNode(const tinygltf::Node& gltfnode)
    {
        // Generate local node matrix
        glm::vec3 translation = glm::vec3(0.0f);
        if(gltfnode.translation.size() == 3)
        {
            Translation = glm::make_vec3(gltfnode.translation.data());
        }
        if(gltfnode.rotation.size() == 4)
        {
            Rotation = glm::make_quat(gltfnode.rotation.data());
        }
        glm::vec3 scale = glm::vec3(1.0f);
        if(gltfnode.scale.size() == 3)
        {
            Scale = glm::make_vec3(gltfnode.scale.data());
        }
        if(gltfnode.matrix.size() == 16)
        {
            Matrix = glm::make_mat4x4(gltfnode.matrix.data());
        };
    }

    void Node::InitFromTinyGltfNode(
        const tinygltf::Model& model, const tinygltf::Node& gltfnode, int32_t index, uint32_t& meshInstanceCount, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
    {
        index   = index;
        mParent = nullptr;
        mName   = gltfnode.name;

        // Node contains mesh data
        if(gltfnode.mesh > -1)
        {
            // TODO: Consider using instancing here!
            mMesh = std::make_unique<MeshInstance>(Owner());
            mMesh->InitFromTinyGltfMesh(model, model.meshes[gltfnode.mesh], meshInstanceCount, indexBuffer, vertexBuffer);
            meshInstanceCount++;
        }
    }

    void Node::ResolveParent()
    {
        if(mParentIndex >= 0)
        {
            mParent = Owner()->GetNodeByIndex(mParentIndex);
            mParent->mChildren.push_back(this);
        }
    }

    glm::mat4 Node::getMatrix()
    {
        // TODO: Save us some CPU work by caching local transformation matrices
        glm::mat4 m = mTransform.LocalMatrix();
        Node*     p = mParent;
        while(p)
        {
            m = p->mTransform.LocalMatrix() * m;
            p = p->mParent;
        }
        return m;
    }

    void Node::update()
    {
        if(mMesh)
        {
            glm::mat4 mat = getMatrix();
            mMesh->Update(mat);
        }
    }

    Node::~Node() {}

    void Node::Draw(SceneDrawInfo& drawInfo) {
        if (mMesh){
            mMesh->Draw(drawInfo);
        }
    }


}  // namespace hsk