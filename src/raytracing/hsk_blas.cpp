#include "hsk_blas.hpp"
#include "../memory/hsk_commandbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/hsk_geo.hpp"
#include <algorithm>

namespace hsk {

    void Blas::Create(const VkContext* context, Mesh* mesh, GeometryStore* store)
    {
        mContext        = context;
        VkDevice device = context->Device;

        auto primitives = mesh->GetPrimitives();

        const uint32_t primitiveCount = primitives.size();


        // // TODO: Get real transform matrices
        // // create tmp transform matrix
        // VkTransformMatrixKHR transform_matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
        // //ManagedBuffer        transformMatrixBuffer;
        // transformMatrixBuffer.Create(
        //     context, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        //     sizeof(transform_matrix), VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        // transformMatrixBuffer.WriteDataDeviceLocal(&transform_matrix, sizeof(transform_matrix));

        VkDeviceOrHostAddressConstKHR vertex_data_device_address{.deviceAddress = store->GetVerticesBuffer().GetDeviceAddress()};
        VkDeviceOrHostAddressConstKHR index_data_device_address{.deviceAddress = store->GetIndicesBuffer().GetDeviceAddress()};
        VkDeviceOrHostAddressConstKHR transform_matrix_device_address{};


        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
        std::vector<uint32_t>                                 primitiveCounts;
        std::vector<VkAccelerationStructureGeometryKHR>       geometries;
        geometries.reserve(primitiveCount);

        for(auto& primitive : primitives)
        {
            VkAccelerationStructureGeometryKHR geometry{};
            // The bottom level acceleration structure contains one set of triangles as the input geometry
            geometry.sType                            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.geometryType                     = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags                            = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.triangles.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData    = vertex_data_device_address;
            geometry.geometry.triangles.maxVertex     = primitive.VertexOrIndexCount;
            geometry.geometry.triangles.vertexStride  = sizeof(Vertex);
            geometry.geometry.triangles.indexType     = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData     = index_data_device_address;
            geometry.geometry.triangles.transformData = transform_matrix_device_address;
            geometries.push_back(geometry);

            // Infer build range info from geometry
            uint32_t primitveCount = primitive.VertexOrIndexCount / 3; // TODO: durch 3 teilen?
            primitiveCounts.push_back(primitveCount);

            // according to the vulkan spec, values behave different, based on which VkGeometryTypeKHR is used.
            // for triangles, as follows:
            VkAccelerationStructureBuildRangeInfoKHR build_range_info;
            build_range_info.primitiveCount  = primitveCount;    // consumes 3x primitiveCount indices
            build_range_info.primitiveOffset = primitive.First * sizeof(uint32_t);  // offset into index buffer  // primitveCount.First?
            build_range_info.firstVertex     = 0;  // added to index values before fetching vertices
            build_range_info.transformOffset = 0;
            buildRangeInfos.push_back(build_range_info);
        }


        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        buildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildGeometryInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
        buildGeometryInfo.pGeometries   = geometries.data();

        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        context->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, primitiveCounts.data(), &buildSizesInfo);


        mBlasMemory.Create(context, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, buildSizesInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mBlasMemory.SetName("Blas memory buffer");

        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
        acceleration_structure_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        acceleration_structure_create_info.buffer = mBlasMemory.GetBuffer();
        acceleration_structure_create_info.size   = buildSizesInfo.accelerationStructureSize;
        acceleration_structure_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        context->DispatchTable.createAccelerationStructureKHR(&acceleration_structure_create_info, nullptr, &mAccelerationStructure);
        buildGeometryInfo.dstAccelerationStructure = mAccelerationStructure;


        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build
        ManagedBuffer scratchBuffer;
        scratchBuffer.Create(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, buildSizesInfo.buildScratchSize,
                             VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT);
        buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        // Build the acceleration structure on the device via a one-time command buffer submission
        // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
        CommandBuffer commandBuffer;
        commandBuffer.Create(context);
        commandBuffer.Begin();
        auto buildRangeInfosPtr = &*buildRangeInfos.data();
        context->DispatchTable.cmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, &buildRangeInfosPtr);
        commandBuffer.Submit();


        // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;
        mBlasAddress                                           = context->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
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