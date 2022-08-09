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

        mAsInstance.accelerationStructureReference = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&addressInfo);
        mTransform->FillVkTransformMatrix(mAsInstance.transform);


        mAsInstance.instanceCustomIndex                    = 0;
        mAsInstance.mask                                   = 0xFF;
        mAsInstance.instanceShaderBindingTableRecordOffset = 0;
        mAsInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    }

    void BlasInstance::Update()
    {
        mTransform->FillVkTransformMatrix(mAsInstance.transform);
    }

    void Tlas::Create()
    {
        mContext = GetContext();

        VkDevice device = mContext->Device;

        auto scene = GetScene();

        std::vector<Node*> nodes;
        scene->FindNodesWithComponent<MeshInstance>(nodes);

        mStaticBlasInstances.clear();
        mAnimatedBlasInstances.clear();
        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;
        VkAccelerationStructureBuildRangeInfoKHR        buildRangeInfo{};
        uint32_t                                        buildCounts = 0;

        for(auto node : nodes)
        {
            MeshInstance*                 meshInstance = node->GetComponent<MeshInstance>();
            std::unique_ptr<BlasInstance> blasInstance = std::make_unique<BlasInstance>(mContext, meshInstance);
            if(blasInstance->GetTransform()->GetStatic())
            {
                mStaticBlasInstances.emplace(meshInstance, std::move(blasInstance));
            }
            else
            {
                mAnimatedBlasInstances.emplace(meshInstance, std::move(blasInstance));
            }
        }

        for(const auto& blasInstancePair : mStaticBlasInstances)
        {
            instanceBufferData.push_back(blasInstancePair.second->GetAsInstance());
            buildCounts++;
        }
        for(const auto& blasInstancePair : mAnimatedBlasInstances)
        {
            instanceBufferData.push_back(blasInstancePair.second->GetAsInstance());
            buildCounts++;
        }

        buildRangeInfo.primitiveCount = buildCounts;

        mInstanceBuffer.Cleanup();
        mInstanceBuffer.SetName("BLAS Instances Buffer");
        mInstanceBuffer.Create(mContext,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                               sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size(), VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        mInstanceBuffer.WriteDataDeviceLocal(instanceBufferData.data(), sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size());

        VkAccelerationStructureGeometryKHR geometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };
        geometry.geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR{
            .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .arrayOfPointers = VK_FALSE,
            .data            = (VkDeviceOrHostAddressConstKHR)mInstanceBuffer.GetDeviceAddress(),
        };


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
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = &geometry;
        buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances (in this case , 1).
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        mContext->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildCounts, &sizeInfo);

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
        mScratchBuffer.Create(mContext, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                             VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        buildInfo.scratchData.deviceAddress = mScratchBuffer.GetDeviceAddress();

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;
        // Build the TLAS.
        mContext->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.Submit();

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }

    void Tlas::Update(const FrameUpdateInfo& drawInfo)
    {
        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;
        VkAccelerationStructureBuildRangeInfoKHR        buildRangeInfo{};
        uint32_t                                        buildCounts = 0;

        for(const auto& blasInstancePair : mAnimatedBlasInstances)
        {
            blasInstancePair.second->Update();
            instanceBufferData.push_back(blasInstancePair.second->GetAsInstance());
            buildCounts++;
        }

        buildRangeInfo.primitiveCount = buildCounts;

        mInstanceBuffer.WriteDataDeviceLocal(instanceBufferData.data(), sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size(),
                                             sizeof(VkAccelerationStructureInstanceKHR) * mStaticBlasInstances.size());

        VkAccelerationStructureGeometryKHR geometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };
        geometry.geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR{
            .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .arrayOfPointers = VK_FALSE,
            .data            = (VkDeviceOrHostAddressConstKHR)mInstanceBuffer.GetDeviceAddress(),
        };


        auto cmdBuffer = drawInfo.GetCommandBuffer();

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        // Create the build info: in this case , pointing to only one
        // geometry object.
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = &geometry;
        buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances (in this case , 1).
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        mContext->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildCounts, &sizeInfo);

        if(mTlasMemory.GetSize() < sizeInfo.accelerationStructureSize)
        {
            mTlasMemory.Cleanup();
            // Allocate a buffer for the acceleration structure.
            mTlasMemory.Create(mContext, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               sizeInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
            mTlasMemory.SetName("Tlas memory buffer");
        }

        buildInfo.dstAccelerationStructure = mAccelerationStructure;
        buildInfo.srcAccelerationStructure = mAccelerationStructure;

        // Allocate the scratch buffer holding temporary build data.
        if(mScratchBuffer.GetSize() < sizeInfo.buildScratchSize)
        {
            mScratchBuffer.Cleanup();
            mScratchBuffer.Create(mContext, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                                  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        }
        buildInfo.scratchData.deviceAddress = mScratchBuffer.GetDeviceAddress();

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;
        // Build the TLAS.
        mContext->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }

    void Tlas::Destroy()
    {
        if(!!mContext && !!mAccelerationStructure)
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