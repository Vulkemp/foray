#include "blas.hpp"
#include "../bench/hostbenchmark.hpp"
#include "../core/commandbuffer.hpp"
#include "../scene/geo.hpp"
#include "../scene/globalcomponents/geometrymanager.hpp"
#include "../scene/mesh.hpp"
#include <algorithm>

namespace foray::as {

    Blas::Blas(core::Context* context, const Builder& builder, bench::HostBenchmark* benchmark)
        : mContext(context)
        , mMesh(builder.GetMesh())
        , mVertexBuffer(builder.GetVertexBuffer())
        , mIndexBuffer(builder.GetIndexBuffer())
    {
        // STEP #0    Reset state
        if(!!benchmark)
        {
            benchmark->Start();
        }

        std::string name = fmt::format("Blas #{:x}", reinterpret_cast<uint64_t>(mMesh));

        mBuildInfo.New(context, mMesh, mVertexBuffer, mIndexBuffer);
        VkAccelerationStructureBuildGeometryInfoKHR& geoInfo = mBuildInfo->GeoInfo;
        VkAccelerationStructureBuildSizesInfoKHR& buildSizes = mBuildInfo->Sizes;

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_CREATESTRUCTS);
        }

        // STEP #2    Fetch build sizes, (re)create buffers

        if(!mBlasMemory || buildSizes.accelerationStructureSize > mBlasMemory->GetSize())
        {
            mBlasMemory.New(mContext, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, buildSizes.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, name);
        }

        std::string                     scratchName = fmt::format("{} scratch", name);
        core::ManagedBuffer::CreateInfo ci(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, buildSizes.buildScratchSize,
                                           VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT, scratchName);
        ci.Alignment = mContext->Device->GetAdapter().AsProperties.minAccelerationStructureScratchOffsetAlignment;
        mScratchMemory.New(mContext, ci);
        geoInfo.scratchData.deviceAddress = mScratchMemory->GetDeviceAddress();

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_GETSIZES);
        }

        // STEP #3    Create the Blas

        VkAccelerationStructureCreateInfoKHR asCi{.sType         = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
                                                  .pNext         = nullptr,
                                                  .createFlags   = 0U,
                                                  .buffer        = mBlasMemory->GetBuffer(),
                                                  .offset        = 0U,
                                                  .size          = buildSizes.accelerationStructureSize,
                                                  .type          = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                                  .deviceAddress = {}};
        mContext->DispatchTable().createAccelerationStructureKHR(&asCi, nullptr, &mAccelerationStructure);

        geoInfo.dstAccelerationStructure = mAccelerationStructure;

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_CREATE);
        }

        // STEP #4   Build the Blas

        core::HostSyncCommandBuffer commandBuffer(context);
        commandBuffer.Begin();
        VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfosPtr = mBuildInfo->BuildRangeInfos.data();
        mContext->DispatchTable().cmdBuildAccelerationStructuresKHR(commandBuffer, 1, &geoInfo, &buildRangeInfosPtr);
        commandBuffer.SubmitAndWait();

        if(!!benchmark)
        {
            benchmark->LogTimestamp(BENCH_BUILD);
        }

        // STEP #5    Get device address
        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;
        mBlasAddress                                           = mContext->DispatchTable().getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);

        if(!!benchmark)
        {
            benchmark->Finalize(BENCH_GETADDRESS);
        }
    }

    Blas::BuildInfo::BuildInfo(core::Context* context, const scene::Mesh* mesh, const core::ManagedBuffer* vertexBuffer, const core::ManagedBuffer* indexBuffer)
    {
        // STEP #1    Build geometries (1 primitve = 1 geometry)
        const std::vector<scene::Primitive>& primitives = mesh->GetPrimitives();
        PrimitiveCount                                  = primitives.size();

        Geometries.resize(PrimitiveCount);
        TriangleCounts.resize(PrimitiveCount);
        BuildRangeInfos.resize(PrimitiveCount);

        VkDeviceOrHostAddressConstKHR vertex_data_device_address{.deviceAddress = vertexBuffer->GetDeviceAddress()};
        VkDeviceOrHostAddressConstKHR index_data_device_address{.deviceAddress = indexBuffer->GetDeviceAddress()};

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

        for(int32_t i = 0; i < (int32_t)PrimitiveCount; i++)
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

            Geometries[i]      = geometry;
            TriangleCounts[i]  = primitveCount;
            BuildRangeInfos[i] = build_range_info;
        }

        // clang-format off
        GeoInfo = VkAccelerationStructureBuildGeometryInfoKHR{
            .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .geometryCount = static_cast<uint32_t>(Geometries.size()),
            .pGeometries   = Geometries.data()};
        // clang-format on

        Sizes = VkAccelerationStructureBuildSizesInfoKHR{.sType = VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        context->DispatchTable().getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &GeoInfo, TriangleCounts.data(), &Sizes);
    }

    Blas::~Blas()
    {
        if(!!mAccelerationStructure)
        {
            mContext->DispatchTable().destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
        }
    }
    void Blas::CmdUpdate(VkCommandBuffer cmdBuffer)
    {
        Assert(mBuildInfo && mScratchMemory);

        VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfosPtr = mBuildInfo->BuildRangeInfos.data();
        mContext->DispatchTable().cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &mBuildInfo->GeoInfo, &buildRangeInfosPtr);
    }
    Blas::Builder& Blas::Builder::SetGeometryStore(scene::gcomp::GeometryStore* store)
    {
        mVertexBuffer = store->GetVerticesBuffer();
        mIndexBuffer  = store->GetIndicesBuffer();
        return *this;
    }
}  // namespace foray::as