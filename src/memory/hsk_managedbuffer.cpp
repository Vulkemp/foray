#include "hsk_managedbuffer.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_singletimecommandbuffer.hpp"
#include "hsk_vmaHelpers.hpp"

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

    void ManagedBuffer::MapAndWrite(void* data, size_t size)
    {
        void* mappedPtr;
        AssertVkResult(vmaMapMemory(mContext->Allocator, mAllocation, &data));
        memcpy(mappedPtr, data, size);
        vmaUnmapMemory(mContext->Allocator, mAllocation);
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

    void ManagedBuffer::WriteDataDeviceLocal(void* data, size_t size, size_t offsetDstBuffer)
    {
        Assert(size + offsetDstBuffer < mAllocationInfo.size, "Attempt to write data to device local buffer failed. Size + offsets needs to fit into buffer allocation!");

        ManagedBuffer           stagingBuffer;
        VmaHelpers::CreateStagingBuffer(&stagingBuffer, mContext, data, size);

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