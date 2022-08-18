#include "hsk_tlas.hpp"
#include "../memory/hsk_commandbuffer.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/components/hsk_transform.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/hsk_node.hpp"
#include "../scenegraph/hsk_scene.hpp"
#include "hsk_blas.hpp"

namespace hsk {

    BlasInstance::BlasInstance(uint64_t instanceId, const Blas* blas, uint64_t blasRef, TransformUpdateFunc getUpdatedGlobalTransformFunc)
        : mInstanceId(instanceId), mGetUpdatedGlobalTransformFunc(getUpdatedGlobalTransformFunc), mAsInstance{}
    {
        mBlas                                              = blas;
        mAsInstance.accelerationStructureReference         = blasRef;
        mAsInstance.instanceCustomIndex                    = 0;
        mAsInstance.mask                                   = 0xFF;
        mAsInstance.instanceShaderBindingTableRecordOffset = 0;
        mAsInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        Update();
    }

    BlasInstance::BlasInstance(uint64_t instanceId, const Blas* blas, uint64_t blasRef, const glm::mat4& globalTransform)
        : mInstanceId(instanceId), mGetUpdatedGlobalTransformFunc(nullptr), mAsInstance{}
    {
        mBlas                                              = blas;
        mAsInstance.accelerationStructureReference         = blasRef;
        mAsInstance.instanceCustomIndex                    = 0;
        mAsInstance.mask                                   = 0xFF;
        mAsInstance.instanceShaderBindingTableRecordOffset = 0;
        mAsInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        TranslateTransformMatrix(globalTransform, mAsInstance.transform);
    }

    void BlasInstance::TranslateTransformMatrix(const glm::mat4& in, VkTransformMatrixKHR& out)
    {
        for(int32_t row = 0; row < 3; row++)
        {
            for(int32_t col = 0; col < 4; col++)
            {
                out.matrix[row][col] = in[col][row];
            }
        }
    }

    void BlasInstance::SetBlasMetaOffset(uint32_t offset)
    {
        mAsInstance.instanceCustomIndex = offset;
    }

    void BlasInstance::Update()
    {
        if(!!mGetUpdatedGlobalTransformFunc)
        {
            glm::mat4 transform;
            std::invoke(mGetUpdatedGlobalTransformFunc, transform);
            TranslateTransformMatrix(transform, mAsInstance.transform);
        }
    }

    Tlas::Tlas(const VkContext* context) : mContext(context) {}

    void Tlas::RemoveBlasInstance(uint64_t id)
    {
        mDirty = true;
        {
            auto iter = mStaticBlasInstances.find(id);
            if(iter != mStaticBlasInstances.end())
            {
                mStaticBlasInstances.erase(iter);
            }
        }
        {
            auto iter = mAnimatedBlasInstances.find(id);
            if(iter != mAnimatedBlasInstances.end())
            {
                mAnimatedBlasInstances.erase(iter);
            }
        }
    }
    BlasInstance* Tlas::GetBlasInstance(uint64_t id)
    {
        {
            auto iter = mStaticBlasInstances.find(id);
            if(iter != mStaticBlasInstances.end())
            {
                return &(iter->second);
            }
        }
        {
            auto iter = mAnimatedBlasInstances.find(id);
            if(iter != mAnimatedBlasInstances.end())
            {
                return &(iter->second);
            }
        }
        return nullptr;
    }
    uint64_t Tlas::AddBlasInstanceAuto(MeshInstance* meshInstance)
    {
        auto        transform = meshInstance->GetNode()->GetTransform();
        const auto& blas      = meshInstance->GetMesh()->GetBlas();

        if(transform->GetStatic())
        {
            transform->RecalculateGlobalMatrix();
            return AddBlasInstanceStatic(blas, transform->GetGlobalMatrix());
        }
        else
        {
            BlasInstance::TransformUpdateFunc getFunc = [transform](glm::mat4& out) { out = transform->GetGlobalMatrix(); };
            return AddBlasInstanceAnimated(blas, getFunc);
        }
    }
    uint64_t Tlas::AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc)
    {
        mDirty      = true;
        uint64_t id = mNextId;
        mNextId++;

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                                .accelerationStructure = blas.GetAccelerationStructure()};

        VkDeviceAddress address = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&addressInfo);

        mAnimatedBlasInstances.emplace(id, BlasInstance(id, &blas, address, getUpdatedGlobalTransformFunc));
        return id;
    }
    uint64_t Tlas::AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform)
    {
        mDirty      = true;
        uint64_t id = mNextId;
        mNextId++;

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
                                                                .accelerationStructure = blas.GetAccelerationStructure()};

        VkDeviceAddress address = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&addressInfo);

        mStaticBlasInstances.emplace(id, BlasInstance(id, &blas, address, transform));
        return id;
    }

    void Tlas::ClearBlasInstances()
    {
        mDirty = true;
        mAnimatedBlasInstances.clear();
        mStaticBlasInstances.clear();
    }


    void Tlas::CreateOrUpdate()
    {
        if(!mDirty)
        {
            return;
        }


        // STEP #0   Reset state
        bool rebuild = mAccelerationStructure != nullptr;


        // STEP #1   Rebuild meta buffer, get and assign buffer offsets
        std::unordered_set<const Blas*> usedBlas; // used to reconstruct the meta info buffer
        for(const auto& blasInstancePair : mStaticBlasInstances)
        {
            usedBlas.emplace(blasInstancePair.second.GetBlas());
        }
        for(const auto& blasInstancePair : mAnimatedBlasInstances)
        {
            usedBlas.emplace(blasInstancePair.second.GetBlas());
        }
        auto offsets = mMetaBuffer.CreateOrUpdate(mContext, usedBlas);

        // STEP #2   Build instance buffer data. Static first, animated after (this way the buffer data that is updated every frame is in memory in one region)
        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;  // Vector of instances (each being a reference to a BLAS, with a transform)

        for(auto& blasInstancePair : mStaticBlasInstances)
        {
            auto& blasInstance = blasInstancePair.second;
            blasInstance.SetBlasMetaOffset(offsets[blasInstance.GetBlas()]);
            instanceBufferData.push_back(blasInstance.GetAsInstance());
        }
        for(auto& blasInstancePair : mAnimatedBlasInstances)
        {
            auto& blasInstance = blasInstancePair.second;
            blasInstance.SetBlasMetaOffset(offsets[blasInstance.GetBlas()]);
            instanceBufferData.push_back(blasInstance.GetAsInstance());
        }

        VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.primitiveCount = instanceBufferData.size();



        // STEP #3   Build instance buffer

        VkDeviceSize instanceBufferSize = instanceBufferData.size() * sizeof(VkAccelerationStructureInstanceKHR);

        if(instanceBufferSize > mInstanceBuffer.GetDeviceBuffer().GetSize())
        {
            VkDeviceSize instanceBufferCapacity = instanceBufferSize + instanceBufferSize / 4;
            mInstanceBuffer.Destroy();
            ManagedBuffer::ManagedBufferCreateInfo instanceBufferCI(
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                instanceBufferCapacity, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, "BLAS Instances Buffer");
            mInstanceBuffer.Create(mContext, instanceBufferCI);
        }

        // Misuse the staging function to upload data to GPU

        mInstanceBuffer.StageSection(0, instanceBufferData.data(), 0, instanceBufferSize);

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
        buildInfo.mode                     = rebuild ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = mAccelerationStructure;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances.
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        mContext->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildRangeInfo.primitiveCount, &sizeInfo);

        // STEP #5    Create main and scratch memory and acceleration structure

        // Allocate a buffer for the acceleration structure.
        if(sizeInfo.accelerationStructureSize > mTlasMemory.GetSize())
        {
            mTlasMemory.Destroy();
            mTlasMemory.Create(mContext, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               sizeInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Tlas main buffer");
        }

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
        if(sizeInfo.buildScratchSize > mScratchBuffer.GetSize())
        {
            mScratchBuffer.Destroy();
            mScratchBuffer.Create(mContext, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                                  VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Tlas Scratch");
        }
        buildInfo.scratchData.deviceAddress = mScratchBuffer.GetDeviceAddress();

        // STEP #6    Build acceleration structure

        CommandBuffer cmdBuffer;
        cmdBuffer.Create(mContext);
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
        mContext->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.Submit();

        // STEP #7    Setup address and staging buffers for animated instances

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = mContext->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);

        mDirty = false;
    }

    void Tlas::UpdateLean(VkCommandBuffer cmdBuffer, uint32_t frameIndex)
    {
        if(!mAnimatedBlasInstances.size())
        {
            return;
        }

        if(mDirty)
        {
            Exception::Throw("Tlas::UpdateLean called when Tlas is in dirty state! Use Tlas::CreateOrUpdate to properly reconfigure!");
        }

        // STEP #1 Grab updated transforms of animated instances and upload them to host buffer

        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;


        for(auto& blasInstancePair : mAnimatedBlasInstances)
        {
            blasInstancePair.second.Update();
            instanceBufferData.push_back(blasInstancePair.second.GetAsInstance());
        }

        size_t writeOffset = sizeof(VkAccelerationStructureInstanceKHR) * mStaticBlasInstances.size();
        size_t writeSize   = sizeof(VkAccelerationStructureInstanceKHR) * mAnimatedBlasInstances.size();
        mInstanceBuffer.StageSection(frameIndex, instanceBufferData.data(), writeOffset, writeSize);

        // STEP #2 Configure upload from host to device buffer for animated instances

        // copy previously staged instance data
        DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT,
                                                     .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                                     .QueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED};
        mInstanceBuffer.CmdCopyToDevice(frameIndex, cmdBuffer, beforeAndAfter, beforeAndAfter);

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

        mContext->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);
    }

    void Tlas::Destroy()
    {
        mStaticBlasInstances.clear();
        mAnimatedBlasInstances.clear();
        if(!!mContext && !!mAccelerationStructure)
        {
            mContext->DispatchTable.destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
            mAccelerationStructure = VK_NULL_HANDLE;
        }
        if(mInstanceBuffer.Exists())
        {
            mInstanceBuffer.Destroy();
        }
        if(mScratchBuffer.Exists())
        {
            mScratchBuffer.Destroy();
        }
        if(mTlasMemory.Exists())
        {
            mTlasMemory.Destroy();
        }
        mTlasAddress = 0;
    }
}  // namespace hsk