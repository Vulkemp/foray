#pragma once
#include "../hsk_basics.hpp"
#include "hsk_rt_declares.hpp"
#include "../hsk_glm.hpp"
#include <functional>

namespace hsk {
    /// @brief A reference to a BLAS maintained by the TLAS
    class BlasInstance
    {
      public:
        /// @brief Function template used for updating the transform matrix of animated instances
        using TransformUpdateFunc = std::function<void(glm::mat4&)>;

        inline BlasInstance() {}
        /// @brief Initialize as animated
        BlasInstance(uint64_t instanceId, const Blas* blas, uint64_t blasRef, TransformUpdateFunc getUpdatedGlobalTransformFunc);
        /// @brief Initialize as static
        BlasInstance(uint64_t instanceId, const Blas* blas, uint64_t blasRef, const glm::mat4& globalTransform);

        HSK_PROPERTY_CGET(InstanceId)
        HSK_PROPERTY_ALLGET(AsInstance)
        HSK_PROPERTY_CGET(Blas)
        HSK_PROPERTY_ALL(GetUpdatedGlobalTransformFunc)

        bool IsAnimated() const { return !!mGetUpdatedGlobalTransformFunc; }

        /// @brief Set the offset into the GeometryMetaBuffer (see hsk_geometrymetabuffer.hpp)
        void SetGeometryMetaOffset(uint32_t offset);

        /// @brief Updates the transform by fetching a new matrix
        void Update();

        /// @brief Translates glms column major 4x4 matrix to the row major 4x3 matrix type VkTransformMatrixKHR
        static void TranslateTransformMatrix(const glm::mat4& in, VkTransformMatrixKHR& out);

      protected:
        /// @brief Id assigned by the TLAS
        uint64_t mInstanceId = 0UL;
        /// @brief BLAS
        const Blas* mBlas;
        /// @brief Function used to update transform (if animated, nullptr otherwise)
        TransformUpdateFunc mGetUpdatedGlobalTransformFunc = nullptr;
        /// @brief Vulkans instance struct, contains the actual reference to the BLAS, the transform and the custom index (used to offset into GeometryMetaBuffer)
        VkAccelerationStructureInstanceKHR mAsInstance = {};
    };
}  // namespace hsk