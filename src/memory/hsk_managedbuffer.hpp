#pragma once
#include "../base/hsk_vkcontext.hpp"
#include <vulkan/vulkan.h>

namespace hsk {

    class ManagedBuffer : public NoMoveDefaults
    {
      public:
        struct ManagedBufferCreateInfo
        {
            const VkContext*        Context;
            VmaAllocationCreateInfo AllocationCreateInfo;
            VkBufferCreateInfo      BufferCreateInfo;

            ManagedBufferCreateInfo();
        };

      public:
        ManagedBuffer() = default;

        void Create(ManagedBufferCreateInfo& createInfo);
        void Destroy();

        void WriteDataDeviceLocal(void* data, size_t size, size_t offset = 0);

        void Map(void*& data);
        void Unmap();

        /// @brief Attempts to 
        /// - map the buffer
        /// - write data given in data ptr with given size
        /// - unmap buffer
        void MapAndWrite(void* data, size_t size);

        inline virtual ~ManagedBuffer()
        {
            if(mAllocation)
            {
                Destroy();
            }
        }

        HSK_PROPERTY_CGET(Buffer);
        HSK_PROPERTY_CGET(IsMapped);
        HSK_PROPERTY_CGET(DescriptorInfo);
        HSK_PROPERTY_CGET(Allocation);

      protected:
        const VkContext*       mContext{};
        VkBuffer               mBuffer{};
        VmaAllocation          mAllocation{};
        VmaAllocationInfo      mAllocationInfo{};
        VkDescriptorBufferInfo mDescriptorInfo = {};
        bool                   mIsMapped       = false;

        void UpdateDescriptorInfo(VkDeviceSize size);
    };

}  // namespace hsk