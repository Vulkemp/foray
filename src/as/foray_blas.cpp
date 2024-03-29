#include "foray_blas.hpp"
#include "../bench/foray_hostbenchmark.hpp"
#include "../core/foray_commandbuffer.hpp"
#include "../scene/foray_geo.hpp"
#include "../scene/foray_mesh.hpp"
#include "../scene/globalcomponents/foray_geometrymanager.hpp"
#include <algorithm>

namespace foray::as {

    void Blas::CreateOrUpdate(core::Context* context, const scene::Mesh* mesh, const scene::gcomp::GeometryStore* store, bench::HostBenchmark* benchmark)
    {
        // STEP #0    Reset state
        if(!!benchmark)
        {
            benchmark->Begin();
        }

        mContext = context;
        mMesh    = mesh;

        std::string name = fmt::format("Blas #{:x}", reinterpret_cast<uint64_t>(mMesh));

        if(mAccelerationStructure != VK_NULL_HANDLE)
        {
            mContext->VkbDispatchTable->destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
            mAccelerationStructure = VK_NULL_HANDLE;
        }
        mBlasAddress = {};

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_RESET);
        }

        VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
        {
            VkPhysicalDeviceProperties2 prop2{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &asProperties};
            vkGetPhysicalDeviceProperties2(mContext->PhysicalDevice(), &prop2);
        }

        // STEP #1    Build geometries (1 primitve = 1 geometry)
        auto           primitives     = mesh->GetPrimitives();
        const uint32_t primitiveCount = primitives.size();

        VkDeviceOrHostAddressConstKHR vertex_data_device_address{.deviceAddress = store->GetVerticesBuffer().GetDeviceAddress()};
        VkDeviceOrHostAddressConstKHR index_data_device_address{.deviceAddress = store->GetIndicesBuffer().GetDeviceAddress()};

        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos(primitiveCount);  // Counterparts to VkCmdDrawIndexed
        std::vector<uint32_t>                                 primitiveCounts(primitiveCount);  // Counts of vertices per geometry, used to determine build size of the BLAS
        std::vector<VkAccelerationStructureGeometryKHR>       geometries(primitiveCount);       // Additional information per geometry/primitive

        // Template geometry with all fields set which are the same for all geometries across this BLAS
        VkAccelerationStructureGeometryKHR geometryTemplate{
            .sType        = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .pNext        = nullptr,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry =
                VkAccelerationStructureGeometryDataKHR{
                    .triangles = VkAccelerationStructureGeometryTrianglesDataKHR{.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                                                                                 .pNext = nullptr,
                                                                                 .vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT,
                                                                                 .vertexData    = vertex_data_device_address,
                                                                                 .vertexStride  = sizeof(scene::Vertex),
                                                                                 .indexType     = VkIndexType::VK_INDEX_TYPE_UINT32,
                                                                                 .indexData     = index_data_device_address,
                                                                                 .transformData = {}}},
            .flags = 0};

        for(int32_t i = 0; i < (int32_t)primitiveCount; i++)
        {
            const auto& primitive = primitives[i];

            VkAccelerationStructureGeometryKHR geometry(geometryTemplate);
            geometry.geometry.triangles.maxVertex = primitive.HighestReferencedIndex;

            uint32_t primitveCount = primitive.VertexOrIndexCount / 3;

            VkAccelerationStructureBuildRangeInfoKHR build_range_info;
            build_range_info.primitiveCount  = primitveCount;                       // consumes 3x primitiveCount indices
            build_range_info.primitiveOffset = primitive.First * sizeof(uint32_t);  // offset into index buffer in bytes
            build_range_info.firstVertex     = 0;                                   // added to index values before fetching vertices
            build_range_info.transformOffset = 0;

            geometries[i]      = geometry;
            primitiveCounts[i] = primitveCount;
            buildRangeInfos[i] = build_range_info;
        }

        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        buildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildGeometryInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
        buildGeometryInfo.pGeometries   = geometries.data();

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_CREATESTRUCTS);
        }

        // STEP #2    Fetch build sizes, (re)create buffers

        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        mContext->VkbDispatchTable->getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, primitiveCounts.data(),
                                                                          &buildSizesInfo);

        if(buildSizesInfo.accelerationStructureSize > mBlasMemory.GetSize())
        {
            mBlasMemory.Destroy();
            mBlasMemory.Create(mContext, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, buildSizesInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0,
                               name);
        }

        core::ManagedBuffer                          scratchBuffer;
        std::string                                  scratchName = fmt::format("{} scratch", name);
        core::ManagedBuffer::CreateInfo ci(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, buildSizesInfo.buildScratchSize,
                                                        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT, scratchName);
        ci.Alignment = asProperties.minAccelerationStructureScratchOffsetAlignment;
        scratchBuffer.Create(mContext, ci);
        buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_GETSIZES);
        }

        // STEP #3    Create the Blas

        VkAccelerationStructureCreateInfoKHR asCi{.sType         = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
                                                  .pNext         = nullptr,
                                                  .createFlags   = 0U,
                                                  .buffer        = mBlasMemory.GetBuffer(),
                                                  .offset        = 0U,
                                                  .size          = buildSizesInfo.accelerationStructureSize,
                                                  .type          = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                  .deviceAddress = {}};
        mContext->VkbDispatchTable->createAccelerationStructureKHR(&asCi, nullptr, &mAccelerationStructure);

        buildGeometryInfo.dstAccelerationStructure = mAccelerationStructure;

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_CREATE);
        }

        // STEP #4   Build the Blas

        core::HostSyncCommandBuffer commandBuffer;
        commandBuffer.Create(context);
        commandBuffer.Begin();
        VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfosPtr = buildRangeInfos.data();
        mContext->VkbDispatchTable->cmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, &buildRangeInfosPtr);
        commandBuffer.SubmitAndWait();

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_BUILD);
        }

        // STEP #5    Get device address
        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;
        mBlasAddress                                           = mContext->VkbDispatchTable->getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);

        if(!!benchmark)
        {
            benchmark->End();
        }
    }

    void Blas::Destroy()
    {
        if(!!mAccelerationStructure)
        {
            mContext->VkbDispatchTable->destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
            mAccelerationStructure = nullptr;
        }
        if(mBlasMemory.Exists())
        {
            mBlasMemory.Destroy();
        }
        mBlasAddress = 0;
    }
}  // namespace foray::as