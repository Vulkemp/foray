#include "hsk_blas.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/hsk_geo.hpp"
#include "../memory/hsk_commandbuffer.hpp"
#include <algorithm>
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"

namespace hsk {

    void Blas::Create(const VkContext* context, Mesh* mesh)
    {
        mContext        = context;
        VkDevice device = context->Device;

        // TODO: Get real transform matrices
        // create tmp transform matrix
        VkTransformMatrixKHR transform_matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
        ManagedBuffer        transformMatrixBuffer;
        transformMatrixBuffer.Create(
            context, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            sizeof(transform_matrix),
                                     VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        transformMatrixBuffer.WriteDataDeviceLocal(&transformMatrixBuffer, sizeof(transform_matrix));

        auto                          geoBufferSet = mesh->GetGeometryBufferSet();
        VkDeviceOrHostAddressConstKHR vertex_data_device_address{.deviceAddress = geoBufferSet->GetVertices().GetDeviceAddress()};
        VkDeviceOrHostAddressConstKHR index_data_device_address{.deviceAddress = geoBufferSet->GetIndices().GetDeviceAddress()};
        VkDeviceOrHostAddressConstKHR transform_matrix_device_address{.deviceAddress = transformMatrixBuffer.GetDeviceAddress()};

        // maxVertex is the highest index a vertex can have, hence get the maximum value from the indices array
        //uint32_t maxVertex = *std::max_element(std::begin(*indices), std::end(*indices));
        uint32_t maxVertex = 1000000;


        // The bottom level acceleration structure contains one set of triangles as the input geometry
        VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
        acceleration_structure_geometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        acceleration_structure_geometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        acceleration_structure_geometry.flags                            = VK_GEOMETRY_OPAQUE_BIT_KHR;
        acceleration_structure_geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        acceleration_structure_geometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
        acceleration_structure_geometry.geometry.triangles.vertexData    = vertex_data_device_address;
        acceleration_structure_geometry.geometry.triangles.maxVertex     = maxVertex;
        acceleration_structure_geometry.geometry.triangles.vertexStride  = sizeof(Vertex);
        acceleration_structure_geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
        acceleration_structure_geometry.geometry.triangles.indexData     = index_data_device_address;
        acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;

        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
        acceleration_structure_build_geometry_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_structure_build_geometry_info.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        acceleration_structure_build_geometry_info.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_structure_build_geometry_info.geometryCount = 1;
        acceleration_structure_build_geometry_info.pGeometries   = &acceleration_structure_geometry;

        const uint32_t primitive_count = 1;

        VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
        acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        context->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &acceleration_structure_build_geometry_info, &primitive_count,
                                                                     &acceleration_structure_build_sizes_info);

        mBlasMemory.Create(context, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, acceleration_structure_build_sizes_info.accelerationStructureSize,
                           VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mBlasMemory.SetName("Blas memory buffer");

        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
        acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        acceleration_structure_create_info.buffer = mBlasMemory.GetBuffer();
        acceleration_structure_create_info.size   = acceleration_structure_build_sizes_info.accelerationStructureSize;
        acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        context->DispatchTable.createAccelerationStructureKHR(&acceleration_structure_create_info, nullptr, &mAccelerationStructure);

        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build
        ManagedBuffer scratchBuffer;
        scratchBuffer.Create(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, acceleration_structure_build_sizes_info.buildScratchSize,
                             VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT);

        VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
        acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        acceleration_build_geometry_info.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        acceleration_build_geometry_info.dstAccelerationStructure  = mAccelerationStructure;
        acceleration_build_geometry_info.geometryCount             = 1;  // TODO: geo count missing?
        acceleration_build_geometry_info.pGeometries               = &acceleration_structure_geometry;
        acceleration_build_geometry_info.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info{};  // TODO: missing infos?
        acceleration_structure_build_range_info.primitiveCount                                          = 1;
        acceleration_structure_build_range_info.primitiveOffset                                         = 0;
        acceleration_structure_build_range_info.firstVertex                                             = 0;
        acceleration_structure_build_range_info.transformOffset                                         = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = {&acceleration_structure_build_range_info};


        // Build the acceleration structure on the device via a one-time command buffer submission
        // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
        CommandBuffer commandBuffer;
        commandBuffer.Create(context);
        commandBuffer.Begin();
        context->DispatchTable.cmdBuildAccelerationStructuresKHR(commandBuffer, 1, &acceleration_build_geometry_info, acceleration_build_structure_range_infos.data());
        commandBuffer.Submit();

        // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;
        mBlasAddress                                          = context->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }
    void Blas::Destroy()
    {
        if(mAccelerationStructure != VK_NULL_HANDLE)
        {
            mContext->DispatchTable.destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
            mAccelerationStructure = VK_NULL_HANDLE;
        }
        if(mBlasMemory.Exists())
        {
            mBlasMemory.Cleanup();
        }
        mBlasAddress = 0;
    }
}  // namespace hsk