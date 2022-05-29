#include "hsk_managedbuffer.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_singletimecommandbuffer.hpp"

namespace hsk {

    void ManagedBuffer::Create(ManagedBufferCreateInfo& createInfo)
    {
        mContext = createInfo.Context;
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

    void ManagedBuffer::Destroy()
    {
        if(mIsMapped)
        {
            logger()->warn("VmaBuffer::Destroy called before Unmap!");
            Unmap();
        }
        if(mContext->Allocator && mAllocation)
        {
            vmaDestroyBuffer(mContext->Allocator, mBuffer, mAllocation);
        }
        mBuffer     = nullptr;
        mAllocation = nullptr;
        UpdateDescriptorInfo(0);
    }

    void ManagedBuffer::WriteDataDeviceLocal(void* data, size_t size, size_t offset)
    {
        Assert(size + offset < mAllocationInfo.size, "Attempt to write data to device local buffer failed. Size + offsets needs to fit into buffer allocation!");

        ManagedBuffer           stagingBuffer;
        ManagedBufferCreateInfo createInfo;
        createInfo.Context                = mContext;
        createInfo.BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.BufferCreateInfo.size  = mAllocationInfo.size;  // size of staging buffer can only be as big as the allocation
        createInfo.BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        createInfo.AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        createInfo.AllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        stagingBuffer.Create(createInfo);


        SingleTimeCommandBuffer singleTimeCmdBuf;
        VkCommandBuffer         commandBuffer = singleTimeCmdBuf.Create(mContext, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copy{};
        copy.srcOffset = offset;
        copy.dstOffset = offset;
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