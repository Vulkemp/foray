#include "hsk_dualbuffer.hpp"
#include "../utility/hsk_hash.hpp"

namespace hsk {
    void DualBuffer::Create(const VkContext* context, const ManagedBuffer::ManagedBufferCreateInfo& devicebufferCreateInfo, uint32_t stageBufferCount)
    {
        Destroy();

        mDeviceBuffer.Create(context, devicebufferCreateInfo);
        mStagingBufferMaps.resize(stageBufferCount, nullptr);
        mBufferCopies.resize(stageBufferCount);

        // Init staging buffers
        ManagedBuffer::ManagedBufferCreateInfo stagingCI(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, devicebufferCreateInfo.BufferCreateInfo.size,
                                                         VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                                                         VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        for(int32_t i = 0; i < static_cast<int32_t>(stageBufferCount); i++)
        {
            auto& stagingBuffer = mStagingBuffers.emplace_back(std::make_unique<ManagedBuffer>());
            auto& mapPoint      = mStagingBufferMaps[i];
            stagingBuffer->Create(context, stagingCI);
            stagingBuffer->Map(mapPoint);
        }
    }

    void DualBuffer::StageSection(uint32_t frameIndex, const void* data, size_t destOffset, size_t size)
    {
        if(!Exists())
        {
            Exception::Throw("DualBuffer::StageDelta/StageFullBuffer called on object in uninitialized state!");
        }

        uint32_t index = frameIndex % mStagingBuffers.size();

        mBufferCopies[index].emplace_back(VkBufferCopy{.srcOffset = destOffset, .dstOffset = destOffset, .size = size});

        memcpy(reinterpret_cast<uint8_t*>(mStagingBufferMaps[index]) + destOffset, data, size);
    }

    void DualBuffer::StageFullBuffer(uint32_t frameIndex, const void* data)
    {
        StageSection(frameIndex, data, 0, mDeviceBuffer.GetSize());
    }

    bool DualBuffer::DeviceBufferState::operator==(const DeviceBufferState& other) const
    {
        return AccessFlags == other.AccessFlags && QueueFamilyIndex == other.QueueFamilyIndex && PipelineStageFlags == other.PipelineStageFlags;
    }

    void DualBuffer::CmdCopyToDevice(
        uint32_t frameIndex, VkCommandBuffer cmdBuffer, const DeviceBufferState& before, const DeviceBufferState& after, uint32_t transferQueueFamilyIndex)
    {
        if(!Exists())
        {
            Exception::Throw("DualBuffer::CmdCopyToDevice called on object in uninitialized state!");
        }

        uint32_t                   index        = frameIndex % mStagingBuffers.size();  // Current in-flight index
        VkBuffer                   source       = mStagingBuffers[index]->GetBuffer();  // Staging buffer containing the delta
        VkBuffer                   dest         = mDeviceBuffer.GetBuffer();            // Buffer the device uses for drawing
        std::vector<VkBufferCopy>& bufferCopies = mBufferCopies[index];                 // copy actions submitted to the device

        VkBufferMemoryBarrier deviceMemBarrier{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, .pNext = nullptr, .buffer = dest, .offset = 0, .size = VK_WHOLE_SIZE};

        if(bufferCopies.size() == 0)
        {
            // Currents frame has no copy operations, so we don't submit any copy commands

            if(before == after)
            {
                // Before and after state are identical, so no pipeline barrier required
                return;
            }

            deviceMemBarrier.srcAccessMask       = before.AccessFlags;
            deviceMemBarrier.dstAccessMask       = after.AccessFlags;
            deviceMemBarrier.srcQueueFamilyIndex = before.QueueFamilyIndex;
            deviceMemBarrier.dstQueueFamilyIndex = after.QueueFamilyIndex;

            vkCmdPipelineBarrier(cmdBuffer, before.PipelineStageFlags, after.PipelineStageFlags, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                                 &deviceMemBarrier, 0, nullptr);

            return;
        }

        // Get proper transfer queue

        if(transferQueueFamilyIndex == TRANSFER_QUEUE_AUTO)
        {
            transferQueueFamilyIndex = before.QueueFamilyIndex;
        }

        // Convert from 'before' state to transfer write state
        VkBufferMemoryBarrier stagingMemBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                                .srcAccessMask       = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT,
                                                .dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT,
                                                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                .buffer              = source,
                                                .offset              = 0,
                                                .size                = VK_WHOLE_SIZE};

        deviceMemBarrier.srcAccessMask       = before.AccessFlags;
        deviceMemBarrier.dstAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        deviceMemBarrier.srcQueueFamilyIndex = before.QueueFamilyIndex;
        deviceMemBarrier.dstQueueFamilyIndex = transferQueueFamilyIndex;

        std::vector<VkBufferMemoryBarrier> barriers{deviceMemBarrier, stagingMemBarrier};

        vkCmdPipelineBarrier(cmdBuffer, before.PipelineStageFlags | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, barriers.size(), barriers.data(), 0, nullptr);

        // Copy
        vkCmdCopyBuffer(cmdBuffer, source, dest, bufferCopies.size(), bufferCopies.data());

        // Convert back from transfer write state to 'after' state
        stagingMemBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
        stagingMemBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;

        deviceMemBarrier.srcAccessMask       = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        deviceMemBarrier.dstAccessMask       = after.AccessFlags;
        deviceMemBarrier.srcQueueFamilyIndex = transferQueueFamilyIndex;
        deviceMemBarrier.dstQueueFamilyIndex = after.QueueFamilyIndex;

        barriers = {deviceMemBarrier, stagingMemBarrier};

        vkCmdPipelineBarrier(cmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, after.PipelineStageFlags | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, 0,
                             nullptr, barriers.size(), barriers.data(), 0, nullptr);

        // Clear "submitted" buffer copies
        bufferCopies.clear();
    }

    void DualBuffer::Destroy()
    {
        mStagingBufferMaps.clear();
        for(auto& stagingBuffer : mStagingBuffers)
        {
            if(stagingBuffer && stagingBuffer->Exists())
            {
                stagingBuffer->Unmap();
            }
        }
        mStagingBuffers.clear();  // Calls destructors
        mBufferCopies.clear();
        mDeviceBuffer.Cleanup();
    }
}  // namespace hsk