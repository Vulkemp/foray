#include "hsk_transform.hpp"
#include "../hsk_node.hpp"

namespace hsk {
    void NTransform::RecalculateLocalMatrix() { mLocalMatrix = glm::translate(glm::mat4(1.0f), mTranslation) * glm::mat4(mRotation) * glm::scale(glm::mat4(1.0f), mScale); }
    void NTransform::RecalculateGlobalMatrix(NTransform* parentTransform)
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

        for(NNode* child : node->GetChildren())
        {
            auto childTransform = child->GetTransform();
            childTransform->RecalculateGlobalMatrix(this);
        }
    }
}  // namespace hsk