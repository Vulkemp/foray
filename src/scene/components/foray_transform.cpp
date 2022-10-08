#include "foray_transform.hpp"
#include "../foray_node.hpp"

namespace foray::scene {
    void Transform::RecalculateLocalMatrix()
    {
        mLocalMatrix = glm::translate(glm::mat4(1.0f), mTranslation) * glm::mat4(mRotation) * glm::scale(glm::mat4(1.0f), mScale);
    }
    void Transform::RecalculateGlobalMatrix(Transform* parentTransform)
    {
        auto node = GetNode();
        if(!node)
        {
            return;
        }

        if(!mLocalMatrixFixed)
            RecalculateLocalMatrix();

        glm::mat4 parentGlobalMatrix(1);
        if(!parentTransform)
        {
            auto parent = node->GetParent();
            if(parent)
            {
                parentTransform    = parent->GetTransform();
                parentGlobalMatrix = parentTransform->GetGlobalMatrix();
            }
        }

        mGlobalMatrix = mLocalMatrix * parentGlobalMatrix;

        for(Node* child : node->GetChildren())
        {
            auto childTransform = child->GetTransform();
            childTransform->RecalculateGlobalMatrix(this);
        }
    }

}  // namespace foray