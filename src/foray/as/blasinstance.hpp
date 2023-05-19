#pragma once
#include "../basics.hpp"
#include "../glm.hpp"
#include "../vulkan.hpp"
#include "as_declares.hpp"
#include <functional>

namespace foray::as {
    /// @brief A reference to a BLAS maintained by the TLAS
    class BlasInstance
    {
      public:
        /// @brief Function template used for updating the transform matrix of animated instances
        using TransformUpdateFunc = std::function<void(glm::mat4&)>;

        inline BlasInstance() {}
        /// @brief Initialize as animated
        BlasInstance(uint64_t instanceId, const Blas* blas, TransformUpdateFunc getUpdatedGlobalTransformFunc);
        /// @brief Initialize as static
        BlasInstance(uint64_t instanceId, const Blas* blas, const glm::mat4& globalTransform);

        FORAY_GETTER_V(InstanceId)
        FORAY_GETTER_CR(AsInstance)
        FORAY_GETTER_V(Blas)
        FORAY_PROPERTY_V(GetUpdatedGlobalTransformFunc)

        bool IsAnimated() const { return !!mGetUpdatedGlobalTransformFunc; }

        /// @brief Set the offset into the GeometryMetaBuffer (see foray_geometrymetabuffer.hpp)
        void SetGeometryMetaOffset(uint32_t offset);

        /// @brief Set the offset for shader binding table
        void SetShaderBindingTableOffset(uint32_t offset);

        inline VkGeometryInstanceFlagsKHR GetGeometryInstanceFlags() const { return mAsInstance.flags; }
        inline BlasInstance&              SetGeometryInstanceFlags(VkGeometryFlagsKHR flags)
        {
            mAsInstance.flags = flags;
            return *this;
        }
        inline BlasInstance& AddGeometryInstanceFlag(VkGeometryFlagBitsKHR flag)
        {
            mAsInstance.flags |= flag;
            return *this;
        }


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
}  // namespace foray::as