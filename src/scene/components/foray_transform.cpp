#include "foray_transform.hpp"
#include "../foray_node.hpp"

namespace foray::scene::ncomp {

    void Transform::SetTranslation(const glm::vec3& translation)
    {
        Assert(!mStatic, "Cannot change translation of node marked static");
        mTranslation = translation;
        SetDirtyRecursively();
    }
    void Transform::SetRotation(const glm::quat& rotation)
    {
        Assert(!mStatic, "Cannot change rotation of node marked static");
        mRotation = rotation;
        SetDirtyRecursively();
    }
    void Transform::SetScale(const glm::vec3& scale)
    {
        Assert(!mStatic, "Cannot change scale of node marked static");
        mScale = scale;
        SetDirtyRecursively();
    }
    void Transform::SetLocalMatrix(const glm::mat4& matrix)
    {
        Assert(!mStatic, "Cannot change local matrix of node marked static");
        mLocalMatrix = matrix;
        SetDirtyRecursively();
    }

    const glm::mat4& Transform::GetGlobalMatrix()
    {
        if(mDirty)
        {
            RecalculateGlobalMatrix();
        }
        return mGlobalMatrix;
    }
    void Transform::RecalculateIfDirty(bool recursive)
    {
        if(mDirty)
        {
            RecalculateGlobalMatrix();
        }
        if(recursive)
        {
            auto node = GetNode();
            if(!node)
            {
                return;
            }
            for(Node* child : node->GetChildren())
            {
                child->GetTransform()->RecalculateIfDirty(true);
            }
        }
    }
    void Transform::SetDirtyRecursively()
    {
        if(mDirty)
        {
            return;
        }
        mDirty    = true;
        auto node = GetNode();
        if(!node)
        {
            return;
        }
        for(Node* child : node->GetChildren())
        {
            child->GetTransform()->SetDirtyRecursively();
        }
    }
    void Transform::RecalculateLocalMatrix()
    {
        mLocalMatrix = glm::translate(glm::mat4(1.0f), mTranslation) * glm::mat4(mRotation) * glm::scale(glm::mat4(1.0f), mScale);
    }
    void Transform::RecalculateGlobalMatrix()
    {
        if(!mLocalMatrixFixed)
        {
            RecalculateLocalMatrix();
        }

        auto node = GetNode();
        if(!node)
        {
            mGlobalMatrix = glm::mat4(1);
        }
        else
        {

            glm::mat4 parentGlobalMatrix(1);
            {
                auto parent = node->GetParent();
                if(parent)
                {
                    Transform* parentTransform = parent->GetTransform();
                    parentGlobalMatrix         = parentTransform->GetGlobalMatrix();
                }
            }
            mGlobalMatrix = parentGlobalMatrix * mLocalMatrix;
        }
        mDirty = false;
    }
}  // namespace foray::scene::ncomp