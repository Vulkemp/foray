#include "hsk_tlas.hpp"
#include "../memory/hsk_commandbuffer.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/components/hsk_transform.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/hsk_node.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "hsk_blas.hpp"

namespace hsk {

    BlasInstance::BlasInstance(const VkContext* context, MeshInstance* meshInstance)
        : mContext(context), mBlas(&(meshInstance->GetMesh()->GetBlas())), mMeshInstance(meshInstance), mTransform(meshInstance->GetNode()->GetTransform())
    {
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = mBlas->GetAccelerationStructure();

        auto& instance = mAsGeometry.geometry.instances;

        mAsInstance.accelerationStructureReference = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&addressInfo);
        mTransform->FillVkTransformMatrix(mAsInstance.transform);


        mAsInstance.instanceCustomIndex                    = 0;
        mAsInstance.mask                                   = 0xFF;
        mAsInstance.instanceShaderBindingTableRecordOffset = 0;
        mAsInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        mAsInstanceBuffer.SetName(fmt::format("BLAS Instance {:x}", reinterpret_cast<uint64_t>(meshInstance)));
        mAsInstanceBuffer.Create(mContext,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                               sizeof(VkAccelerationStructureInstanceKHR), VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        mAsInstanceBuffer.WriteDataDeviceLocal(&mAsInstance, sizeof(VkAccelerationStructureInstanceKHR));

        mAsGeometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        mAsGeometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        mAsGeometry.flags                                 = VK_GEOMETRY_OPAQUE_BIT_KHR;
        mAsGeometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        mAsGeometry.geometry.instances.arrayOfPointers    = VK_FALSE;
        mAsGeometry.geometry.instances.data.deviceAddress = mAsInstanceBuffer.GetDeviceAddress();
    }

    void Tlas::Create()
    {
        mContext = GetContext();

        VkDevice device = mContext->Device;

        auto scene = GetScene();

        std::vector<Node*> nodes;
        scene->FindNodesWithComponent<MeshInstance>(nodes);

        mBlasInstances.clear();
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
        uint32_t maxBuildCounts = 0;

        for(auto node : nodes)
        {
            MeshInstance* meshInstance = node->GetComponent<MeshInstance>();
            BlasInstance* blasInstance = mBlasInstances.emplace(meshInstance, std::make_unique<BlasInstance>(mContext, meshInstance)).first->second.get();
            geometries.push_back(blasInstance->GetAsGeometry());
            buildRangeInfos.push_back(VkAccelerationStructureBuildRangeInfoKHR{.primitiveCount=1});
        }


        CommandBuffer cmdBuffer;
        cmdBuffer.Create(mContext);
        cmdBuffer.Begin();

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        // Create the build info: in this case , pointing to only one
        // geometry object.
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = geometries.data();
        buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances (in this case , 1).
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        mContext->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &maxBuildCounts, &sizeInfo);

        // Allocate a buffer for the acceleration structure.
        mTlasMemory.Create(mContext, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           sizeInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mTlasMemory.SetName("Tlas memory buffer");

        // Create the acceleration structure object.
        // (Data has not yet been set.)
        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type   = buildInfo.type;
        createInfo.size   = sizeInfo.accelerationStructureSize;
        createInfo.buffer = mTlasMemory.GetBuffer();
        createInfo.offset = 0;

        AssertVkResult(mContext->DispatchTable.createAccelerationStructureKHR(&createInfo, nullptr, &mAccelerationStructure));
        buildInfo.dstAccelerationStructure = mAccelerationStructure;

        // Allocate the scratch buffer holding temporary build data.
        ManagedBuffer bufferScratch;
        bufferScratch.Create(mContext, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                             VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        buildInfo.scratchData.deviceAddress = bufferScratch.GetDeviceAddress();

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = buildRangeInfos.data();
        // Build the TLAS.
        mContext->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.Submit();

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }
    void Tlas::Destroy()
    {
        if(mAccelerationStructure != VK_NULL_HANDLE)
        {
            mContext->DispatchTable.destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
            mAccelerationStructure = VK_NULL_HANDLE;
        }
        if(mTlasMemory.Exists())
        {
            mTlasMemory.Cleanup();
        }
        mTlasAddress = 0;
    }
}  // namespace hsk