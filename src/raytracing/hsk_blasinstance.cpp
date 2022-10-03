#include "hsk_blasinstance.hpp"
#include "hsk_blas.hpp"

namespace hsk {
    BlasInstance::BlasInstance(uint64_t instanceId, const Blas* blas, TransformUpdateFunc getUpdatedGlobalTransformFunc)
        : mInstanceId(instanceId), mGetUpdatedGlobalTransformFunc(getUpdatedGlobalTransformFunc), mAsInstance{}
    {
        mBlas                                              = blas;
        mAsInstance.accelerationStructureReference         = blas->GetBlasAddress();
        mAsInstance.instanceCustomIndex                    = 0;
        mAsInstance.mask                                   = 0xFF;
        mAsInstance.instanceShaderBindingTableRecordOffset = 0;
        mAsInstance.flags                                  = 0;

        Update();
    }

    BlasInstance::BlasInstance(uint64_t instanceId, const Blas* blas, const glm::mat4& globalTransform)
        : mInstanceId(instanceId), mGetUpdatedGlobalTransformFunc(nullptr), mAsInstance{}
    {
        mBlas                                              = blas;
        mAsInstance.accelerationStructureReference         = blas->GetBlasAddress();
        mAsInstance.instanceCustomIndex                    = 0;
        mAsInstance.mask                                   = 0xFF;
        mAsInstance.instanceShaderBindingTableRecordOffset = 0;
        mAsInstance.flags                                  = 0;

        TranslateTransformMatrix(globalTransform, mAsInstance.transform);
    }

    void BlasInstance::TranslateTransformMatrix(const glm::mat4& in, VkTransformMatrixKHR& out)
    {
        for(int32_t row = 0; row < 3; row++)
        {
            for(int32_t col = 0; col < 4; col++)
            {
                out.matrix[row][col] = in[col][row];
            }
        }
    }

    void BlasInstance::SetGeometryMetaOffset(uint32_t offset)
    {
        mAsInstance.instanceCustomIndex = offset;
    }

    void BlasInstance::SetShaderBindingTableOffset(uint32_t offset)
    {
        mAsInstance.instanceShaderBindingTableRecordOffset = offset;
    }

    void BlasInstance::Update()
    {
        if(!!mGetUpdatedGlobalTransformFunc)
        {
            glm::mat4 transform;
            std::invoke(mGetUpdatedGlobalTransformFunc, transform);
            TranslateTransformMatrix(transform, mAsInstance.transform);
        }
    }
}  // namespace hsk
