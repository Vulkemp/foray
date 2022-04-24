#include "hsk_vmaHelpers.hpp"
#include "base/hsk_logger.hpp"
#include "hsk_vkHelpers.hpp"

namespace hsk {
    VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBufferLevel level, bool begin)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool        = cmdpool;
        cmdBufAllocateInfo.level              = level;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        AssertVkResult(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

        // If requested, also start recording for the new command buffer
        if(begin)
        {
            VkCommandBufferBeginInfo commandBufferBI{};
            commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            AssertVkResult(vkBeginCommandBuffer(cmdBuffer, &commandBufferBI));
        }

        return cmdBuffer;
    }

    void beginCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        AssertVkResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBI));
    }

    void flushCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBuffer commandBuffer, VkQueue queue, bool free)
    {
        AssertVkResult(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        AssertVkResult(vkCreateFence(device, &fenceInfo, nullptr, &fence));

        // Submit to the queue
        AssertVkResult(vkQueueSubmit(queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        AssertVkResult(vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000));

        vkDestroyFence(device, fence, nullptr);

        if(free)
        {
            vkFreeCommandBuffers(device, cmdpool, 1, &commandBuffer);
        }
    }

    void createBuffer(
        VmaAllocator allocator, VkBufferUsageFlags usageFlags, VmaAllocationCreateInfo allocInfo, VmaAllocation* allocation, VkDeviceSize size, VkBuffer* buffer, void* data)
    {

        // Create the buffer handle
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.usage       = usageFlags;
        bufferCreateInfo.size        = size;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        AssertVkResult(vmaCreateBuffer(allocator, &bufferCreateInfo, &allocInfo, buffer, allocation, nullptr));
        // If a pointer to the buffer data has been passed, map the buffer and copy over the data
        if(data != nullptr)
        {
            void* mapped;
            AssertVkResult(vmaMapMemory(allocator, *allocation, &mapped));
            memcpy(mapped, data, size);
            // TODO: flushing a mapped memory range is currently not supported..?
            //// If host coherency hasn't been requested, do a manual flush to make writes visible
            //if((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            //{
            //    VkMappedMemoryRange mappedRange{};
            //    mappedRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            //    mappedRange.memory = *memory;
            //    mappedRange.offset = 0;
            //    mappedRange.size   = size;
            //    vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
            //}
            vmaUnmapMemory(allocator, *allocation);
        }
    }

    ManagedBuffer& ManagedBuffer::Allocator(VmaAllocator allocator)
    {
        AssertLoaded(false, "Allocator (Setter)");
        mAllocator = allocator;
        return *this;
    }


    void ManagedBuffer::Init(VmaAllocator allocator, VkBufferUsageFlags usageFlags, VmaAllocationCreateInfo allocInfo, VkDeviceSize size, void* data)
    {
        mAllocator = allocator;
        Init(usageFlags, allocInfo, size, data);
    }
    void ManagedBuffer::Init(VkBufferUsageFlags usageFlags, VmaAllocationCreateInfo allocInfo, VkDeviceSize size, void* data)
    {
        createBuffer(mAllocator, usageFlags, allocInfo, &mAllocation, size, &mBuffer, data);
        UpdateDescriptorInfo(size);
    }

    void ManagedBuffer::Map(void*& data)
    {
        AssertLoaded(true, "Map");
        AssertVkResult(vmaMapMemory(mAllocator, mAllocation, &data));
        mIsMapped = true;
    }

    void ManagedBuffer::Unmap()
    {
        if(!mAllocation)
        {
            logger()->warn("VmaBuffer::Unmap called on uninitialized buffer!");
        }
        vmaUnmapMemory(mAllocator, mAllocation);
        mIsMapped = false;
    }

    void ManagedBuffer::Destroy()
    {
        if(mIsMapped)
        {
            logger()->warn("VmaBuffer::Destroy called before Unmap!");
            Unmap();
        }
        vmaDestroyBuffer(mAllocator, mBuffer, mAllocation);
        mBuffer     = nullptr;
        mAllocation = nullptr;
        UpdateDescriptorInfo(0);
    }

    void ManagedBuffer::AssertLoaded(bool loaded, const char* process)
    {
        bool isloaded = (bool)mAllocation;
        if(loaded != isloaded)
        {
            throw Exception("VmaBuffer::{} requires the buffer to be {}!", process, loaded ? "loaded" : "uninitialized");
        }
    }

    void ManagedBuffer::UpdateDescriptorInfo(VkDeviceSize size)
    {
        mDescriptorInfo = {};
        mDescriptorInfo.buffer = mBuffer;
        mDescriptorInfo.offset = 0;
        mDescriptorInfo.range = size;
    }

}  // namespace hsk