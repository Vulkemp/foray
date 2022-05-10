#pragma once
#include "../hsk_basics.hpp"
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {
    class IntermediateBuffer : public NoMoveDefaults
    {
      public:
        inline IntermediateBuffer() {}
        inline ~IntermediateBuffer() { Destroy(); }

        struct CreateInfo
        {
            VkImageCreateInfo       ImageCI{};
            VmaAllocationCreateInfo AllocCI{};

            CreateInfo();
        };

        inline virtual void Init(VkDevice device, VmaAllocator allocator, const CreateInfo& createInfo)
        {
            mDevice    = device;
            mAllocator = allocator;
            Init(createInfo);
        }
        virtual void Init(const CreateInfo& createInfo);

        virtual void Destroy();

        HSK_PROPERTY_CGET(Image)
        HSK_PROPERTY_CGET(ImageView)
        HSK_PROPERTY_CGET(Allocation)
        HSK_PROPERTY_CGET(AllocInfo)
        HSK_PROPERTY_CGET(Format)
        HSK_PROPERTY_ALL(Device)
        HSK_PROPERTY_ALL(Allocator)

      protected:
        VkImage           mImage;
        VkImageView       mImageView;
        VkFormat          mFormat;
        VmaAllocation     mAllocation;
        VmaAllocationInfo mAllocInfo;
        VkDevice          mDevice;
        VmaAllocator      mAllocator;
    };
}  // namespace hsk