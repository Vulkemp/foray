#pragma once
#include "../hsk_basics.hpp"
#include <vulkan/vulkan.h>
#include "../memory/hsk_managedbuffer.hpp"

namespace hsk {

    class Blas;

    /// @brief Describes a top level accerlation structure. A tlas usually holds multiple Blas.
    /// A blas is an object/mesh instance together with its position/rotation in the 3d space.
    class Tlas : public NoMoveDefaults
    {
      public:
        virtual ~Tlas() { Destroy(); }

        virtual void Create(const VkContext* context, Blas& blas);
        virtual void Destroy();

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(TlasMemory)
        HSK_PROPERTY_CGET(TlasAddress)

        operator VkAccelerationStructureKHR() { return mAccelerationStructure; }

      protected:
        const VkContext* mContext{};
        VkAccelerationStructureKHR mAccelerationStructure;
        ManagedBuffer              mTlasMemory;
        VkDeviceAddress            mTlasAddress{};
    };
}  // namespace hsk