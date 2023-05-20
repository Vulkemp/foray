#include "tlas.hpp"
#include "../core/commandbuffer.hpp"
#include "../scene/components/meshinstance.hpp"
#include "../scene/components/transform.hpp"
#include "../scene/mesh.hpp"
#include "../scene/node.hpp"
#include "../scene/scene.hpp"
#include "blas.hpp"
#include "blasinstance.hpp"

namespace foray::as {

    Tlas::Builder& Tlas::Builder::AddBlasInstance(scene::ncomp::MeshInstance* meshInstance)
    {
        uint64_t key;
        return AddBlasInstance(meshInstance, key);
    }
    Tlas::Builder& Tlas::Builder::AddBlasInstance(scene::ncomp::MeshInstance* meshInstance, uint64_t& OUT_key)
    {
        auto      transform = meshInstance->GetNode()->GetTransform();
        as::Blas* blas      = meshInstance->GetMesh()->GetBlas();

        if(transform->GetStatic())
        {
            AddBlasInstanceStatic(*blas, transform->GetGlobalMatrix(), OUT_key);
        }
        else
        {
            BlasInstance::TransformUpdateFunc getFunc = [transform](glm::mat4& out) { out = transform->GetGlobalMatrix(); };
            AddBlasInstanceAnimated(*blas, getFunc, OUT_key);
        }
        return *this;
    }
    Tlas::Builder& Tlas::Builder::AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc)
    {
        uint64_t key;
        return AddBlasInstanceAnimated(blas, getUpdatedGlobalTransformFunc, key);
    }
    Tlas::Builder& Tlas::Builder::AddBlasInstanceAnimated(const Blas& blas, BlasInstance::TransformUpdateFunc getUpdatedGlobalTransformFunc, uint64_t& OUT_key)
    {
        mAnimatedBlasInstances.emplace(mNextKey, BlasInstance(mNextKey, &blas, getUpdatedGlobalTransformFunc));
        OUT_key = mNextKey;
        mNextKey++;
        return *this;
    }
    Tlas::Builder& Tlas::Builder::AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform)
    {
        uint64_t key;
        return AddBlasInstanceStatic(blas, transform, key);
    }
    Tlas::Builder& Tlas::Builder::AddBlasInstanceStatic(const Blas& blas, const glm::mat4& transform, uint64_t& OUT_key)
    {
        mStaticBlasInstances.emplace(mNextKey, BlasInstance(mNextKey, &blas, transform));
        OUT_key = mNextKey;
        mNextKey++;
        return *this;
    }
    BlasInstance* Tlas::Builder::FindBlasInstance(uint64_t key)
    {
        {
            auto iter = mStaticBlasInstances.find(key);
            if(iter != mStaticBlasInstances.end())
            {
                return &(iter->second);
            }
        }
        {
            auto iter = mAnimatedBlasInstances.find(key);
            if(iter != mAnimatedBlasInstances.end())
            {
                return &(iter->second);
            }
        }
        return nullptr;
    }
    Tlas::Builder& Tlas::Builder::RemoveBlasInstance(uint64_t key)
    {
        {
            auto iter = mStaticBlasInstances.find(key);
            if(iter != mStaticBlasInstances.end())
            {
                mStaticBlasInstances.erase(iter);
            }
        }
        {
            auto iter = mAnimatedBlasInstances.find(key);
            if(iter != mAnimatedBlasInstances.end())
            {
                mAnimatedBlasInstances.erase(iter);
            }
        }
        return *this;
    }

    Tlas::Tlas(core::Context* context, const Builder& builder)
        : mContext(context)
        , mAccelerationStructure(builder.GetAccelerationStructure())
        , mAnimatedBlasInstances(builder.GetAnimatedBlasInstances())
        , mStaticBlasInstances(builder.GetStaticBlasInstances())
    {
        // STEP #1   Rebuild meta buffer, get and assign buffer offsets
        std::unordered_set<const Blas*> usedBlas;  // used to reconstruct the meta info buffer
        for(const auto& blasInstancePair : mStaticBlasInstances)
        {
            usedBlas.emplace(blasInstancePair.second.GetBlas());
        }
        for(const auto& blasInstancePair : mAnimatedBlasInstances)
        {
            usedBlas.emplace(blasInstancePair.second.GetBlas());
        }
        mMetaBuffer.New(mContext, usedBlas);

        // STEP #2   Build instance buffer data. Static first, animated after (this way the buffer data that is updated every frame is in memory in one region)
        std::vector<VkAccelerationStructureInstanceKHR> instanceBufferData;  // Vector of instances (each being a reference to a BLAS, with a transform)

        for(auto& blasInstancePair : mStaticBlasInstances)
        {
            auto& blasInstance = blasInstancePair.second;
            blasInstance.SetGeometryMetaOffset(mMetaBuffer->GetOffsetOf(blasInstance.GetBlas()));
            instanceBufferData.push_back(blasInstance.GetAsInstance());
        }
        for(auto& blasInstancePair : mAnimatedBlasInstances)
        {
            auto& blasInstance = blasInstancePair.second;
            blasInstance.SetGeometryMetaOffset(mMetaBuffer->GetOffsetOf(blasInstance.GetBlas()));
            instanceBufferData.push_back(blasInstance.GetAsInstance());
        }

        VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.primitiveCount = instanceBufferData.size();


        // STEP #3   Build instance buffer

        VkDeviceSize instanceBufferSize = instanceBufferData.size() * sizeof(VkAccelerationStructureInstanceKHR);

        core::ManagedBuffer::CreateInfo instanceBufferCI(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, instanceBufferSize,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, "BLAS Instances Buffer");
        mInstanceBuffer.New(mContext, instanceBufferCI);

        // Misuse the staging function to upload data to GPU

        mInstanceBuffer->StageFullBuffer(0, instanceBufferData.data());

        // STEP #4    Get Size

        VkAccelerationStructureGeometryKHR geometry{
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR,
        };
        geometry.geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR{
            .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .arrayOfPointers = VK_FALSE,
            .data            = VkDeviceOrHostAddressConstKHR{.deviceAddress = mInstanceBuffer->GetDeviceBuffer().GetDeviceAddress()},
        };

        // Create the build info. The spec limits this to a single geometry, containing all instance references!
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = &geometry;
        buildInfo.mode                     = !!mAccelerationStructure ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = mAccelerationStructure;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances.
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        mContext->DispatchTable().getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &buildRangeInfo.primitiveCount, &sizeInfo);

        // STEP #5    Create main and scratch memory and acceleration structure

        // Allocate a buffer for the acceleration structure.
        mTlasMemory.New(mContext, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        sizeInfo.accelerationStructureSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Tlas main buffer");

        // Create the acceleration structure object.
        // (Data has not yet been set.)
        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type   = buildInfo.type;
        createInfo.size   = sizeInfo.accelerationStructureSize;
        createInfo.buffer = mTlasMemory->GetBuffer();
        createInfo.offset = 0;

        AssertVkResult(mContext->DispatchTable().createAccelerationStructureKHR(&createInfo, nullptr, &mAccelerationStructure));
        buildInfo.dstAccelerationStructure = mAccelerationStructure;

        // Allocate the scratch buffer holding temporary build data.
        core::ManagedBuffer::CreateInfo ci(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                                           VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Tlas Scratch");
        ci.Alignment = mContext->Device->GetProperties().AsProperties.minAccelerationStructureScratchOffsetAlignment;
        mScratchBuffer.New(mContext, ci);
        buildInfo.scratchData.deviceAddress = mScratchBuffer->GetDeviceAddress();

        // STEP #6    Build acceleration structure

        core::HostSyncCommandBuffer cmdBuffer(mContext);
        cmdBuffer.Begin();

        mInstanceBuffer->CmdCopyToDevice(0, cmdBuffer);

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;
        // Build the TLAS.
        mContext->DispatchTable().cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.SubmitAndWait();

        // STEP #7    Setup address and staging buffers for animated instances

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = mContext->DispatchTable().getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
    }

    void Tlas::CmdUpdate(VkCommandBuffer cmdBuffer, uint32_t frameIndex)
    {
        if(!mAnimatedBlasInstances.size())
        {
            return;
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
        mInstanceBuffer->StageSection(frameIndex, instanceBufferData.data(), writeOffset, writeSize);

        // STEP #2 Configure upload from host to device buffer for animated instances

        // copy previously staged instance data
        mInstanceBuffer->CmdCopyToDevice(frameIndex, cmdBuffer);

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
            .data            = VkDeviceOrHostAddressConstKHR{.deviceAddress = mInstanceBuffer->GetDeviceBuffer().GetDeviceAddress()},
        };

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        buildInfo.geometryCount             = 1;
        buildInfo.pGeometries               = &geometry;
        buildInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
        buildInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.dstAccelerationStructure  = mAccelerationStructure;
        buildInfo.srcAccelerationStructure  = mAccelerationStructure;
        buildInfo.scratchData.deviceAddress = mScratchBuffer->GetDeviceAddress();


        VkAccelerationStructureBuildRangeInfoKHR  buildRangeInfo{.primitiveCount = static_cast<uint32_t>(mAnimatedBlasInstances.size() + mStaticBlasInstances.size())};
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &buildRangeInfo;

        mContext->DispatchTable().cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);
    }

    Tlas::~Tlas()
    {
        mContext->DispatchTable().destroyAccelerationStructureKHR(mAccelerationStructure, nullptr);
    }
}  // namespace foray::as