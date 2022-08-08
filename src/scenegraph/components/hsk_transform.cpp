#include "hsk_transform.hpp"
#include "../hsk_node.hpp"

namespace hsk {
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

        if(!mStatic)
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

    void Transform::FillVkTransformMatrix(VkTransformMatrixKHR& mat)
    {
        for (int32_t row = 0; row < 3; row++){
            for (int32_t col = 0; col < 4; col++){
                mat.matrix[row][col] = mGlobalMatrix[col][row];
            }
        }
    }
}  // namespace hsk