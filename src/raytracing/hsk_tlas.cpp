#include "hsk_tlas.hpp"
#include "../memory/hsk_commandbuffer.hpp"
#include "hsk_blas.hpp"

namespace hsk {

    void Tlas::Create(const VkContext* context, Blas& blas)
    {
        mContext = context;

        VkDevice                                    device = context->Device;
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = blas.GetAccelerationStructure();
        VkDeviceAddress blasAddress       = context->DispatchTable.getAccelerationStructureDeviceAddressKHR(&addressInfo);

        VkTransformMatrixKHR transform_matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

        VkAccelerationStructureInstanceKHR instance{};

        instance.transform                              = transform_matrix;
        instance.instanceCustomIndex                    = 0;
        instance.mask                                   = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference         = blasAddress;

        ManagedBuffer bufferInstances;
        bufferInstances.SetName("Temporary rt instances buffer");
        bufferInstances.Create(context,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                               sizeof(instance), VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        bufferInstances.WriteDataDeviceLocal(&instance, sizeof(instance));

        CommandBuffer cmdBuffer;
        cmdBuffer.Create(context);
        cmdBuffer.Begin();

        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        // Like creating the BLAS , point to the geometry (in this case , the
        // instances) in a polymorphic object.
        VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
        acceleration_structure_geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        acceleration_structure_geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        acceleration_structure_geometry.flags                                 = VK_GEOMETRY_OPAQUE_BIT_KHR;
        acceleration_structure_geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        acceleration_structure_geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
        acceleration_structure_geometry.geometry.instances.data.deviceAddress = bufferInstances.GetDeviceAddress();


        // Create the build info: in this case , pointing to only one
        // geometry object.
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        buildInfo.flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount            = 1;
        buildInfo.pGeometries              = &acceleration_structure_geometry;
        buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
        rangeInfo.primitiveOffset = 0;
        rangeInfo.primitiveCount  = 1;  // Number of instances
        rangeInfo.firstVertex     = 0;
        rangeInfo.transformOffset = 0;

        // Query the worst -case AS size and scratch space size based on
        // the number of instances (in this case , 1).
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        context->DispatchTable.getAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &rangeInfo.primitiveCount, &sizeInfo);

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

        AssertVkResult(context->DispatchTable.createAccelerationStructureKHR(&createInfo, nullptr, &mAccelerationStructure));
        buildInfo.dstAccelerationStructure = mAccelerationStructure;

        // Allocate the scratch buffer holding temporary build data.
        ManagedBuffer bufferScratch;
        bufferScratch.Create(mContext, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeInfo.buildScratchSize,
                             VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        buildInfo.scratchData.deviceAddress = bufferScratch.GetDeviceAddress();

        // Create a one -element array of pointers to range info objects.
        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;
        // Build the TLAS.
        context->DispatchTable.cmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pRangeInfo);

        cmdBuffer.Submit();

        VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
        acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        acceleration_device_address_info.accelerationStructure = mAccelerationStructure;

        mTlasAddress = context->DispatchTable.getAccelerationStructureDeviceAddressKHR(&acceleration_device_address_info);
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