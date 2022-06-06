#include "hsk_scenetransformbuffer.hpp"

namespace hsk {
    SceneTransformBuffer::SceneTransformBuffer(const VkContext* context) : mBuffer(context, false) { mBuffer.GetBuffer().SetName("TransformBuffer"); }

    void SceneTransformBuffer::Resize(size_t size)
    {
        mBuffer.GetVector().resize(size);
        mTouchedTransforms.resize(size);
    }
    void SceneTransformBuffer::UpdateSceneTransform(int32_t meshInstanceIndex, const glm::mat4& modelMatrix)
    {
        auto& transformVector = mBuffer.GetVector();

        HSK_ASSERTFMT(meshInstanceIndex >= 0 && meshInstanceIndex < transformVector.size(),
                      "SceneTransformBuffer::UpdateSceneTransform: parameter meshInstanceIndex out of bounds. Allowed values: [0, {}[", transformVector.size())

        auto& modelTransform               = transformVector[meshInstanceIndex];
        modelTransform.PreviousModelMatrix = modelTransform.ModelMatrix;
        modelTransform.ModelMatrix         = modelMatrix;

        mTouchedTransforms[meshInstanceIndex] = true;
    }

    void SceneTransformBuffer::BeforeDraw(const FrameRenderInfo& renderInfo)
    {
        auto& transformVector = mBuffer.GetVector();

        for(int32_t i = 0; i < mTouchedTransforms.size(); i++)
        {
            if(!mTouchedTransforms[i])
            {
                auto& modelTransform               = transformVector[i];
                modelTransform.PreviousModelMatrix = modelTransform.ModelMatrix;
            }
            mTouchedTransforms[i] = false;
        }

        mBuffer.InitOrUpdate();
    }
}  // namespace hsk