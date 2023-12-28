#include "dualbuffer.hpp"
//#include "hash.hpp"

namespace foray::util {
    DualBuffer::DualBuffer(core::Context* context, const core::ManagedBuffer::CreateInfo& devicebufferCreateInfo, uint32_t stageBufferCount)
     : mDeviceBuffer(context, devicebufferCreateInfo)
    {
        mStagingBufferMaps.resize(stageBufferCount, nullptr);
        mBufferCopies.resize(stageBufferCount);

        // Init staging buffers
        core::ManagedBuffer::CreateInfo stagingCI(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, devicebufferCreateInfo.BufferCreateInfo.size,
                                                  VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                                                  VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        for(int32_t i = 0; i < static_cast<int32_t>(stageBufferCount); i++)
        {
            if(devicebufferCreateInfo.Name.size() > 0)
            {
                stagingCI.Name = fmt::format("Staging for \"{}\" #{}", devicebufferCreateInfo.Name, i);
            }
            auto& stagingBuffer = mStagingBuffers.emplace_back(context, stagingCI);
            auto& mapPoint      = mStagingBufferMaps[i];
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

    void DualBuffer::CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer)
    {
        if(!Exists())
        {
            Exception::Throw("DualBuffer::CmdCopyToDevice called on object in uninitialized state!");
        }

        uint32_t                   index        = frameIndex % mStagingBuffers.size();  // Current in-flight index
        vk::Buffer                   source       = mStagingBuffers[index]->GetBuffer();  // Staging buffer containing the delta
        vk::Buffer                   dest         = mDeviceBuffer.GetBuffer();            // Buffer the device uses for drawing
        std::vector<VkBufferCopy>& bufferCopies = mBufferCopies[index];                 // copy actions submitted to the device

        if(bufferCopies.size() == 0)
        {
            // Currents frame has no copy operations, so we don't submit any copy commands
            return;
        }

        {  // Buffer memory barriers

            VkBufferMemoryBarrier2 stagingMemBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                                     .srcStageMask        = VK_PIPELINE_STAGE_2_HOST_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                     .srcAccessMask       = VK_ACCESS_2_HOST_WRITE_BIT,
                                                     .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                     .dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT,
                                                     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                     .buffer              = source,
                                                     .size                = VK_WHOLE_SIZE};

            VkBufferMemoryBarrier2 deviceMemBarrier{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                                    .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                                    .srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT,
                                                    .dstStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                    .dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                    .buffer              = dest,
                                                    .size                = VK_WHOLE_SIZE};


            std::vector<VkBufferMemoryBarrier2> barriers{deviceMemBarrier, stagingMemBarrier};

            VkDependencyInfo depInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .bufferMemoryBarrierCount = (uint32_t)barriers.size(), .pBufferMemoryBarriers = barriers.data()};

            vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
        }

        {  // Copy
            vkCmdCopyBuffer(cmdBuffer, source, dest, bufferCopies.size(), bufferCopies.data());
        }

        // Clear "submitted" buffer copies
        bufferCopies.clear();
    }

    void DualBuffer::CmdPrepareForRead(VkCommandBuffer cmdBuffer, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask) const
    {
        VkBufferMemoryBarrier2 barrier = MakeBarrierPrepareForRead(dstStageMask, dstAccessMask);

        VkDependencyInfo depInfo{.sType = VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .bufferMemoryBarrierCount = 1U, .pBufferMemoryBarriers = &barrier};

        vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
    }

    VkBufferMemoryBarrier2 DualBuffer::MakeBarrierPrepareForRead(vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask) const
    {
        return VkBufferMemoryBarrier2{.sType               = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                      .srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                      .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                      .dstStageMask        = dstStageMask,
                                      .dstAccessMask       = dstAccessMask,
                                      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                      .buffer              = mDeviceBuffer.GetBuffer(),
                                      .size                = VK_WHOLE_SIZE};
    }

    DualBuffer& DualBuffer::SetName(std::string_view name)
    {
        mDeviceBuffer.SetName(name);
        std::string stagingBufferName;
        for(int32_t i = 0; i < (int32_t)mStagingBuffers.size(); i++)
        {
            stagingBufferName = fmt::format("Staging for \"{}\" #{}", name, i);
            mStagingBuffers[i]->SetName(stagingBufferName);
        }
        return *this;
    }
}  // namespace foray::util