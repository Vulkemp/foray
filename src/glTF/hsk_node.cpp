#include "hsk_node.hpp"
#include "hsk_mesh.hpp"
#include "hsk_skin.hpp"

namespace hsk {
    glm::mat4 Node::localMatrix() { return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix; }

    glm::mat4 Node::getMatrix()
    {
        glm::mat4 m = localMatrix();
        Node*     p = parent;
        while(p)
        {
            m = p->localMatrix() * m;
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
                    Node* jointNode           = skin->joints[i];
                    glm::mat4     jointMat            = jointNode->getMatrix() * skin->inverseBindMatrices[i];
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

    Node::~Node()
    {
        for(auto& child : children)
        {
            delete child;
        }
    }


}  // namespace hsk