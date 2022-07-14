#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../memory/hsk_managedbuffer.hpp"

namespace hsk {

    // hsk_geo Vertex struct prototype
    struct Vertex;
    class Mesh;

    class Blas
    {
      public:
        virtual ~Blas() { Destroy(); }

        virtual void Create(const VkContext* context, Mesh* mesh);
        virtual void Destroy();

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(BlasAddress)
        HSK_PROPERTY_CGET(BlasMemory)

        operator VkAccelerationStructureKHR() { return mAccelerationStructure; }

      protected:
        const VkContext* mContext;
        ManagedBuffer    mBlasMemory;
        VkAccelerationStructureKHR mAccelerationStructure{};
        VkDeviceAddress            mBlasAddress{};
    };
}  // namespace hsk