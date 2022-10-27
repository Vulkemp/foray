#pragma once
#include "../foray_vulkan.hpp"
#include "../foray_vma.hpp"
#include "foray_commandbuffer.hpp"
#include "foray_managedresource.hpp"
#include "foray_context.hpp"

namespace foray::core {

    class ManagedBuffer : public VulkanResource<VkObjectType::VK_OBJECT_TYPE_BUFFER>
    {
      public:
        struct ManagedBufferCreateInfo
        {
            VmaAllocationCreateInfo AllocationCreateInfo{};
            VkBufferCreateInfo      BufferCreateInfo{};
            VkDeviceSize            Alignment{};
            std::string             Name{};

            ManagedBufferCreateInfo();
            ManagedBufferCreateInfo(VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = {}, std::string_view name = "");
        };

      public:
        ManagedBuffer() : VulkanResource("Unnamed Buffer") { };

        void Create(Context* context, const ManagedBufferCreateInfo& createInfo);
        /// @brief Creates the buffer with VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT. If data is set, also maps writes and unmaps
        void CreateForStaging(Context* context, VkDeviceSize size, const void* data = nullptr, std::string_view bufferName = {});
        /// @brief Simplified version of Create that omits the use of a create info but should be sufficient for many usecases
        void Create(
            Context* context, VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = {}, std::string_view name = "");

        virtual void Destroy() override;

        virtual bool Exists() const override { return mAllocation; }

        void WriteDataDeviceLocal(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
        /// @brief Create staging buffer, copy data to it, copy data to device buffer, destroy staging buffer
        /// @param cmdBuffer Single use command buffer. Must be completely reset, function will automatically begin, write and submit.
        void WriteDataDeviceLocal(HostCommandBuffer& cmdBuffer, const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        void Map(void*& data);
        void Unmap();

        /// @brief Attempts to
        /// - map the buffer
        /// - write data given in data ptr with given size
        /// - unmap buffer
        /// If size is left to 0, attempts to write full buffer size.
        void MapAndWrite(const void* data, size_t size = 0);

        inline virtual ~ManagedBuffer()
        {
            if(mAllocation)
            {
                Destroy();
            }
        }

        FORAY_PROPERTY_CGET(Buffer);
        FORAY_PROPERTY_CGET(IsMapped);
        FORAY_PROPERTY_CGET(Allocation);
        FORAY_PROPERTY_CGET(AllocationInfo);
        FORAY_PROPERTY_CGET(Name);
        FORAY_PROPERTY_CGET(Size);

        VkDeviceAddress GetDeviceAddress() const;

        virtual void SetName(std::string_view name) override;

        inline VkDescriptorBufferInfo GetVkDescriptorBufferInfo() const { return VkDescriptorBufferInfo{.buffer = mBuffer, .offset = 0, .range = mSize}; }
        void                          FillVkDescriptorBufferInfo(VkDescriptorBufferInfo* bufferInfo) const;

      protected:
        Context*  mContext{};
        VkBuffer          mBuffer{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocationInfo{};
        VkDeviceSize      mSize      = {};
        VkDeviceSize      mAlignment = {};
        bool              mIsMapped  = false;

        void UpdateDebugNames();
    };

}  // namespace foray::core