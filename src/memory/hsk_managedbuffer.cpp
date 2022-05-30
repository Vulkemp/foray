#include "hsk_managedbuffer.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_singletimecommandbuffer.hpp"
#include "hsk_vmaHelpers.hpp"

namespace hsk {

    void ManagedBuffer::Create(const VkContext* context, ManagedBufferCreateInfo& createInfo)
    {
        mContext = context;
        vmaCreateBuffer(mContext->Allocator, &createInfo.BufferCreateInfo, &createInfo.AllocationCreateInfo, &mBuffer, &mAllocation, &mAllocationInfo);
    }

    void ManagedBuffer::CreateForStaging(const VkContext* context, VkDeviceSize size, void* data)
    {
        Create(context, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                         VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        if(data)
        {
            MapAndWrite(data);
        }
    }

    void ManagedBuffer::Create(const VkContext* context, VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags)
    {
        mContext = context;
        ManagedBufferCreateInfo createInfo;
        createInfo.BufferCreateInfo.size      = size;
        createInfo.BufferCreateInfo.usage     = usage;
        createInfo.AllocationCreateInfo.usage = memoryUsage;
        createInfo.AllocationCreateInfo.flags = flags;

        vmaCreateBuffer(mContext->Allocator, &createInfo.BufferCreateInfo, &createInfo.AllocationCreateInfo, &mBuffer, &mAllocation, &mAllocationInfo);
    }

    void ManagedBuffer::Map(void*& data)
    {
        AssertVkResult(vmaMapMemory(mContext->Allocator, mAllocation, &data));
        mIsMapped = true;
    }

    void ManagedBuffer::Unmap()
    {
        if(!mAllocation)
        {
            logger()->warn("VmaBuffer::Unmap called on uninitialized buffer!");
        }
        vmaUnmapMemory(mContext->Allocator, mAllocation);
        mIsMapped = false;
    }

    void ManagedBuffer::MapAndWrite(void* data)
    {
        void* mappedPtr = nullptr;
        AssertVkResult(vmaMapMemory(mContext->Allocator, mAllocation, &mappedPtr));
        memcpy(mappedPtr, data, (size_t)mAllocationInfo.size);
        vmaUnmapMemory(mContext->Allocator, mAllocation);
    }

    void ManagedBuffer::Destroy()
    {
        if(mIsMapped)
        {
            logger()->warn("ManagedBuffer::Destroy called before Unmap!");
            Unmap();
        }
        if(mContext && mContext->Allocator && mAllocation)
        {
            vmaDestroyBuffer(mContext->Allocator, mBuffer, mAllocation);
        }
        mBuffer     = nullptr;
        mAllocation = nullptr;
        UpdateDescriptorInfo(0);
    }

    void ManagedBuffer::WriteDataDeviceLocal(void* data, VkDeviceSize size, VkDeviceSize offsetDstBuffer)
    {
        Assert(size + offsetDstBuffer < mAllocationInfo.size, "Attempt to write data to device local buffer failed. Size + offsets needs to fit into buffer allocation!");

        ManagedBuffer stagingBuffer;
        stagingBuffer.CreateForStaging(mContext, size, data);

        SingleTimeCommandBuffer singleTimeCmdBuf;
        VkCommandBuffer         commandBuffer = singleTimeCmdBuf.Create(mContext, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = offsetDstBuffer;
        copy.size      = size;

        vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), mBuffer, 1, &copy);
        singleTimeCmdBuf.Flush();
    }

    void ManagedBuffer::UpdateDescriptorInfo(VkDeviceSize size)
    {
        mDescriptorInfo        = {};
        mDescriptorInfo.buffer = mBuffer;
        mDescriptorInfo.offset = 0;
        mDescriptorInfo.range  = size;
    }

    ManagedBuffer::ManagedBufferCreateInfo::ManagedBufferCreateInfo() { BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; }

}  // namespace hsk