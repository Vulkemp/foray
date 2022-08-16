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

        mInstanceBuffer.Destroy();
        ManagedBuffer::ManagedBufferCreateInfo instanceBufferCI(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                                                    | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                                                sizeof(VkAccelerationStructureInstanceKHR) * instanceBufferData.size(), VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, "BLAS Instances Buffer");
        mInstanceBuffer.Create(GetContext(), instanceBufferCI);

        // Misuse the staging function to upload data to GPU

        mInstanceBuffer.StageFullBuffer(0, instanceBufferData.data());

        // STEP #4    Get Size

        VkAccelerationStructureGeometryKHR geometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };
        geometry.geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR{
            .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .arrayOfPointers = VK_FALSE,
            .data            = (VkDeviceOrHostAddressConstKHR)mInstanceBuffer.GetDeviceBuffer().GetDeviceAddress(),
        };

        // Create the build info. The spec limits this to a single geometry, containing all instance references!
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = &geometry;
        buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances.
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        GetContext()->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildRangeInfo.primitiveCount, &sizeInfo);

        // STEP #5    Create main and scratch memory and acceleration structure

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

        // STEP #6    Build acceleration structure

        CommandBuffer cmdBuffer;
        cmdBuffer.Create(GetContext());
        cmdBuffer.Begin();

        // copy previously staged instance data
        DualBuffer::DeviceBufferState before{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
                                             .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                                             .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        DualBuffer::DeviceBufferState after{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                                            .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                            .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};

        mInstanceBuffer.CmdCopyToDevice(0, cmdBuffer, before, after);

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;
        // Build the TLAS.
        GetContext()->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.Submit();

        // STEP #7    Setup address and staging buffers for animated instances

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = GetContext()->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }

    void Tlas::Update(const FrameUpdateInfo& drawInfo)
    {
        if(!mAnimatedBlasInstances.size())
        {
            return;
        }

        // STEP #1 Grab updated transforms of animated instances and upload them to host buffer

        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;


        for(const auto& blasInstancePair : mAnimatedBlasInstances)
        {
            blasInstancePair.second->Update();
            instanceBufferData.push_back(blasInstancePair.second->GetAsInstance());
        }

        size_t writeOffset = sizeof(VkAccelerationStructureInstanceKHR) * mStaticBlasInstances.size();
        size_t writeSize   = sizeof(VkAccelerationStructureInstanceKHR) * mAnimatedBlasInstances.size();
        mInstanceBuffer.StageSection(drawInfo.GetFrameNumber(), instanceBufferData.data(), writeOffset, writeSize);

        auto cmdBuffer = drawInfo.GetCommandBuffer();

        // STEP #2 Configure upload from host to device buffer for animated instances

        // copy previously staged instance data
        DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT,
                                                     .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                                     .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        mInstanceBuffer.CmdCopyToDevice(drawInfo.GetFrameNumber(), cmdBuffer, beforeAndAfter, beforeAndAfter);

        // STEP #3 Rebuild/Update TLAS

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        VkAccelerationStructureGeometryKHR geometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };
        geometry.geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR{
            .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .arrayOfPointers = VK_FALSE,
            .data            = (VkDeviceOrHostAddressConstKHR)mInstanceBuffer.GetDeviceBuffer().GetDeviceAddress(),
        };

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount             = 1;
        buildInfo.pGeometries               = &geometry;
        buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.dstAccelerationStructure  = mAccelerationStructure;
        buildInfo.srcAccelerationStructure  = mAccelerationStructure;
        buildInfo.scratchData.deviceAddress = mScratchBuffer.GetDeviceAddress();


        VkAccelerationStructureBuildRangeInfoKHR  buildRangeInfo{.primitiveCount = static_cast<uint32_t>(mAnimatedBlasInstances.size() + mStaticBlasInstances.size())};
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;

        GetContext()->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);
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
        if(mInstanceBuffer.Exists())
        {
            mInstanceBuffer.Destroy();
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