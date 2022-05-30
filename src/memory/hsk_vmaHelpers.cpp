#include "hsk_vmaHelpers.hpp"
#include "../base/hsk_logger.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_managedbuffer.hpp"

namespace hsk {
    void VmaHelpers::CreateStagingBuffer(ManagedBuffer* outManagedBuffer, const VkContext* context, void* data, size_t size)
    {
        ManagedBuffer::ManagedBufferCreateInfo createInfo;
        createInfo.Context                = context;
        createInfo.BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.BufferCreateInfo.size  = size;
        createInfo.BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.AllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        createInfo.AllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        outManagedBuffer->Create(createInfo);

        outManagedBuffer->MapAndWrite(data, size);
    }
}  // namespace hsk