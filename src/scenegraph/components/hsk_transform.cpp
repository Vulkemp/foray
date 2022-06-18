#include "hsk_transform.hpp"
#include "../hsk_node.hpp"

namespace hsk {
    void Transform::RecalculateLocalMatrix()
    {
        if(!mStatic)
            mLocalMatrix = glm::translate(glm::mat4(1.0f), mTranslation) * glm::mat4(mRotation) * glm::scale(glm::mat4(1.0f), mScale);
    }
    void Transform::RecalculateGlobalMatrix(Transform* parentTransform)
    {
        auto node = GetNode();
        if(!node)
        {
            return;
        }

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
}  // namespace hsk