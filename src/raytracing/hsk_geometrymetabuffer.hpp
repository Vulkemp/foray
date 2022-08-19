#pragma
#include "../memory/hsk_descriptorsethelper.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "hsk_rt_declares.hpp"
#include <unordered_map>
#include <unordered_set>

namespace hsk {
    /// @brief Meta information for a Geometry (gltf equivalent = Primitive). For use in raytracing shaders.
    struct GeometryMeta
    {
        /// @brief Index into the materialbuffer
        int32_t MaterialIndex = 0;
        /// @brief Index into the vertex buffer (combine with gl_PrimitiveID to get the correct indices from the indexbuffer)
        uint32_t IndexBufferOffset = 0U;
    };

    /// @brief Device local buffer maintaining GeometryMeta structs for all primitives/geometries for multiple BLAS
    class GeometryMetaBuffer
    {
        /* DATA LAYOUT / USAGE

        initialize with a set of BLAS. BLAS are assigned sections in the buffer consecutively:

          | BLAS #0           | BLAS #1      | BLAS #2                     | // Unique BLAS

        The amount of space assigned per BLAS is GeometryCount * sizeof(GeometryMeta)

          | BLAS #0           | BLAS #1      | BLAS #2                     | // Unique BLAS
          | G0 | G1 | G2 | G3 | G0 | G1 | G2 | G0 | G1 | G2 | G3 | G4 | G5 | // Geometry Meta structs (1x per Geometry per BLAS)

        The buffer maintains the offsets into this buffer for each BLAS

          | BLAS #0           | BLAS #1      | BLAS #2                     | // Unique BLAS
          | G0 | G1 | G2 | G3 | G0 | G1 | G2 | G0 | G1 | G2 | G3 | G4 | G5 | // Geometry Meta structs (1x per Geometry per BLAS)
            ^                   ^              ^
            |                   |              |
            0                   4              7                             // Offset per BLAS into the Geometry Meta Array

        These offsets are the return value of GeometryMetaBuffer::CreateOrUpdate and can be later retrieved via GeometryMetaBuffer::GetBufferOffsets()

        Intended use for these offsets is to set them as VkAccelerationStructureInstanceKHR::instanceCustomIndex in BLAS instances installed into a TLAS.
        In rt shaders the correct meta struct can be accessed by 
        geoMetaindex = gl_InstanceCustomIndexEXT + gl_GeometryIndexEXT;

        gl_InstanceCustomIndexEXT: The custom index returned by the GeometryMetaBuffer and set as part of the BLAS instance struct in the TLAS
        gl_GeometryIndexEXT: The index of the geometry within the current BLAS

        */
      public:
        /// @brief (re)creates the meta buffer
        const std::unordered_map<const Blas*, uint32_t>& CreateOrUpdate(const VkContext* context, const std::unordered_set<const Blas*>& entries);

        HSK_PROPERTY_ALLGET(Buffer)
        HSK_PROPERTY_CGET(BufferOffsets)

        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(VkShaderStageFlags shaderStage);

      protected:
        const VkContext*                          mContext = nullptr;
        /// @brief Maps BLAS to their offsets into the BlasMetaBuffers GeometryMeta array
        std::unordered_map<const Blas*, uint32_t> mBufferOffsets;
        /// @brief The device local buffer holding the array
        ManagedBuffer                             mBuffer;
        std::vector<VkDescriptorBufferInfo>       mDescriptorInfos;
    };
}  // namespace hsk