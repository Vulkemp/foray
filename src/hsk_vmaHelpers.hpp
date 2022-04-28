#pragma once
#include "hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBufferLevel level, bool begin = false);

    void beginCommandBuffer(VkCommandBuffer commandBuffer);

    void flushCommandBuffer(VkDevice device, VkCommandPool cmdpool, VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

    void createBuffer(VmaAllocator            allocator,
                      VkBufferUsageFlags      usageFlags,
                      VmaAllocationCreateInfo allocInfo,
                      VmaAllocation*          allocation,
                      VkDeviceSize            size,
                      VkBuffer*               buffer,
                      void*                   data = nullptr);

    class ManagedBuffer : public NoMoveDefaults
    {
      public:
        inline ManagedBuffer() {}
        inline ManagedBuffer(VmaAllocator allocator) : mAllocator(allocator) {}

        void Init(VmaAllocator allocator, VkBufferUsageFlags usageFlags, VmaAllocationCreateInfo allocInfo, VkDeviceSize size, void* data = nullptr);
        void Init(VkBufferUsageFlags usageFlags, VmaAllocationCreateInfo allocInfo, VkDeviceSize size, void* data = nullptr);
        void Destroy();

        void Map(void*& data);
        void Unmap();

        inline virtual ~ManagedBuffer()
        {
            if(mAllocation)
            {
                Destroy();
            }
        }

        HSK_PROPERTY_GET(Allocator);
        HSK_PROPERTY_CGET(Allocator);
        ManagedBuffer& Allocator(VmaAllocator allocator);

        HSK_PROPERTY_CGET(Buffer);
        HSK_PROPERTY_CGET(Allocation);
        HSK_PROPERTY_CGET(IsMapped);
        HSK_PROPERTY_CGET(DescriptorInfo);

      protected:
        VmaAllocator           mAllocator      = nullptr;
        VkBuffer               mBuffer         = nullptr;
        VmaAllocation          mAllocation     = nullptr;
        VkDescriptorBufferInfo mDescriptorInfo = {};
        bool                   mIsMapped       = false;

        void AssertLoaded(bool loaded, const char* process);
        void UpdateDescriptorInfo(VkDeviceSize size);
    };
}  // namespace hsk
