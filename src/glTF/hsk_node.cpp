#include "hsk_node.hpp"
#include "hsk_mesh.hpp"
#include "hsk_scene.hpp"
#include "hsk_skin.hpp"

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
        const tinygltf::Model& model, const tinygltf::Node& gltfnode, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
    {
        index     = index;
        parent    = nullptr;
        name      = gltfnode.name;
        skinIndex = gltfnode.skin;


        // Node contains mesh data
        if(gltfnode.mesh > -1)
        {
            // TODO: Consider using instancing here!
            mesh = std::make_unique<Mesh>(mOwningScene);
            mesh->InitFromTinyGltfMesh(model, model.meshes[gltfnode.mesh], indexBuffer, vertexBuffer);
        }
    }

    void Node::ResolveParent()
    {
        if(parentIndex >= 0)
        {
            parent = mOwningScene->GetNodeByIndex(parentIndex);
            parent->children.push_back(this);
        }
    }

    glm::mat4 Node::getMatrix()
    {
        // TODO: Save us some CPU work by caching local transformation matrices
        glm::mat4 m = mTransform.LocalMatrix();
        Node*     p = parent;
        while(p)
        {
            m = p->mTransform.LocalMatrix() * m;
            p = p->parent;
        }
        return m;
    }

    void Node::update()
    {
        if(mesh)
        {
            glm::mat4 m = getMatrix();
            if(skin)
            {
                mesh->uniformBlock.matrix = m;
                // Update join matrices
                glm::mat4 inverseTransform = glm::inverse(m);
                size_t    numJoints        = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
                for(size_t i = 0; i < numJoints; i++)
                {
                    Node*     jointNode               = skin->joints[i];
                    glm::mat4 jointMat                = jointNode->getMatrix() * skin->inverseBindMatrices[i];
                    jointMat                          = inverseTransform * jointMat;
                    mesh->uniformBlock.jointMatrix[i] = jointMat;
                }
                mesh->uniformBlock.jointcount = (float)numJoints;
                memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
            }
            else
            {
                memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
            }
        }

        for(auto& child : children)
        {
            child->update();
        }
    }

    Node::~Node() {}


}  // namespace hsk