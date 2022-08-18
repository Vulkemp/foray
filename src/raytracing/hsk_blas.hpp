#pragma once
#include "../base/hsk_vkcontext.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../scenegraph/hsk_scenegraph_declares.hpp"

namespace hsk {

    // hsk_geo Vertex struct prototype
    struct Vertex;
    class Mesh;

    class Blas
    {
      public:
        virtual ~Blas() { Destroy(); }

        virtual void Create(const VkContext* context, Mesh* mesh, GeometryStore* store);
        virtual void Destroy();

        HSK_PROPERTY_CGET(AccelerationStructure)
        HSK_PROPERTY_CGET(BlasAddress)
        HSK_PROPERTY_CGET(BlasMemory)
        HSK_PROPERTY_CGET(Mesh)

      protected:
        const VkContext*           mContext = nullptr;
        Mesh*                mMesh    = nullptr;
        ManagedBuffer              mBlasMemory;
        VkAccelerationStructureKHR mAccelerationStructure{};
        VkDeviceAddress            mBlasAddress{};
    };
}  // namespace hsk