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
        // STEP #0   Reset state
        Destroy();

        // STEP #1   Find all references to meshes ( ~= BLAS), filter static/animated
        std::vector<Node*> nodes;
        GetScene()->FindNodesWithComponent<MeshInstance>(nodes);

        for(auto node : nodes)
        {
            MeshInstance*                 meshInstance = node->GetComponent<MeshInstance>();
            std::unique_ptr<BlasInstance> blasInstance = std::make_unique<BlasInstance>(GetContext(), meshInstance);
            if(blasInstance->GetTransform()->GetStatic())
            {
                mStaticBlasInstances.emplace(meshInstance, std::move(blasInstance));
            }
            else
            {
                mAnimatedBlasInstances.emplace(meshInstance, std::move(blasInstance));
            }
        }

        // STEP #2   Build instance buffer data. Static first, animated after (this way the buffer data that is updated every frame is in memory in one region)

        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;  // Vector of instances (each being a reference to a BLAS, with a transform)

        for(const auto& blasInstancePair : mStaticBlasInstances)
        {
            instanceBufferData.push_back(blasInstancePair.second->GetAsInstance());
        }
        for(const auto& blasInstancePair : mAnimatedBlasInstances)
        {
            instanceBufferData.push_back(blasInstancePair.second->GetAsInstance());
        }

        VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.primitiveCount = instanceBufferData.size();

        // STEP #3   Build instance buffer

        mInstanceBuffer.Cleanup();
        mInstanceBuffer.SetName("BLAS Instances Buffer");
        mInstanceBuffer.Create(GetContext(),
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                               sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size(), VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        mInstanceBuffer.WriteDataDeviceLocal(instanceBufferData.data(), sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size());

        // STEP #4    Build

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

        // Create the build info. The spec limits this to a single geometry, containing all instance references!
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = &geometry;
        buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        CommandBuffer cmdBuffer;
        cmdBuffer.Create(GetContext());
        cmdBuffer.Begin();

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        // Query the worst -case AS size and scratch space size based on
        // the number of instances.
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        GetContext()->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildRangeInfo.primitiveCount, &sizeInfo);

        // Allocate a buffer for the acceleration structure.
        mTlasMemory.Create(GetContext(), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           sizeInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mTlasMemory.SetName("Tlas memory buffer");

        // Create the acceleration structure object.
        // (Data has not yet been set.)
        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type   = buildInfo.type;
        createInfo.size   = sizeInfo.accelerationStructureSize;
        createInfo.buffer = mTlasMemory.GetBuffer();
        createInfo.offset = 0;

        AssertVkResult(GetContext()->DispatchTable.createAccelerationStructureKHR(&createInfo, nullptr, &mAccelerationStructure));
        buildInfo.dstAccelerationStructure = mAccelerationStructure;

        // Allocate the scratch buffer holding temporary build data.
        mScratchBuffer.Create(GetContext(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                              VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        buildInfo.scratchData.deviceAddress = mScratchBuffer.GetDeviceAddress();

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;
        // Build the TLAS.
        GetContext()->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.Submit();

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = GetContext()->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);

        if(!!mAnimatedBlasInstances.size())
        {
            // Create and map Staging buffers used for runtime updates
            for(uint32_t i = 0; i < INFLIGHT_FRAME_COUNT; i++)
            {
                mAnimatedInstancesStaging[i].SetName(fmt::format("BLAS Animated Instances Staging #{}", i));
                mAnimatedInstancesStaging[i].Create(GetContext(), VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    sizeof(VkAccelerationStructureInstanceKHR) * mAnimatedBlasInstances.size(), VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                                                    VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
                mAnimatedInstancesStaging[i].Map(mAnimatedInstancesStagingMaps[i]);
            }
        }
    }

    void Tlas::Update(const FrameUpdateInfo& drawInfo)
    {
        if(!mAnimatedBlasInstances.size())
        {
            return;
        }

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


        memcpy(mAnimatedInstancesStagingMaps[drawInfo.GetFrameNumber() % INFLIGHT_FRAME_COUNT], instanceBufferData.data(),
               sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size());

        auto cmdBuffer = drawInfo.GetCommandBuffer();

        VkBufferMemoryBarrier instanceBufferBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                                    .srcAccessMask       = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT,
                                                    .dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
                                                    .srcQueueFamilyIndex = GetContext()->QueueGraphics,
                                                    .dstQueueFamilyIndex = GetContext()->QueueGraphics,
                                                    .buffer              = mInstanceBuffer.GetBuffer(),
                                                    .offset              = 0,
                                                    .size                = VK_WHOLE_SIZE};

        vkCmdPipelineBarrier(cmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &instanceBufferBarrier, 0, nullptr);

        VkBufferCopy region{.srcOffset = 0,
                            .dstOffset = sizeof(VkAccelerationStructureInstanceKHR) * mStaticBlasInstances.size(),
                            .size      = sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size()};

        vkCmdCopyBuffer(cmdBuffer, mAnimatedInstancesStaging[drawInfo.GetFrameNumber()].GetBuffer(), mInstanceBuffer.GetBuffer(), 1, &region);

        instanceBufferBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        instanceBufferBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &instanceBufferBarrier, 0, nullptr);

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
        GetContext()->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildCounts, &sizeInfo);

        if(mTlasMemory.GetSize() < sizeInfo.accelerationStructureSize)
        {
            mTlasMemory.Cleanup();
            // Allocate a buffer for the acceleration structure.
            mTlasMemory.Create(GetContext(),
                               VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               sizeInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
            mTlasMemory.SetName("Tlas memory buffer");
        }

        buildInfo.dstAccelerationStructure = mAccelerationStructure;
        buildInfo.srcAccelerationStructure = mAccelerationStructure;

        // Allocate the scratch buffer holding temporary build data.
        if(mScratchBuffer.GetSize() < sizeInfo.buildScratchSize)
        {
            mScratchBuffer.Cleanup();
            mScratchBuffer.Create(GetContext(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                                  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        }
        buildInfo.scratchData.deviceAddress = mScratchBuffer.GetDeviceAddress();

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;
        // Build the TLAS.
        GetContext()->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = GetContext()->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }

    void Tlas::Destroy()
    {
        mStaticBlasInstances.clear();
        mAnimatedBlasInstances.clear();
        mTlasAddress = {};
        if(!!GetContext() && !!mAccelerationStructure)
        {
            GetContext()->DispatchTable.destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
            mAccelerationStructure = VK_NULL_HANDLE;
        }
        for(uint32_t i = 0; i < INFLIGHT_FRAME_COUNT; i++)
        {
            if(mAnimatedInstancesStaging[i].Exists())
            {
                mAnimatedInstancesStaging[i].Unmap();
                mAnimatedInstancesStaging[i].Cleanup();
            }
        }
        if(mInstanceBuffer.Exists())
        {
            mInstanceBuffer.Cleanup();
        }
        if(mScratchBuffer.Exists())
        {
            mScratchBuffer.Cleanup();
        }
        if(mTlasMemory.Exists())
        {
            mTlasMemory.Cleanup();
        }
        mTlasAddress = 0;
    }
}  // namespace hsk