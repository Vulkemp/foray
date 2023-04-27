#pragma once
#include "../foray_vma.hpp"
#include "../foray_vulkan.hpp"
#include "foray_commandbuffer.hpp"
#include "foray_context.hpp"
#include "foray_managedresource.hpp"

namespace foray::core {

    /// @brief Wraps allocation and lifetime functionality of a VkBuffer
    class ManagedBuffer : public VulkanResource<VkObjectType::VK_OBJECT_TYPE_BUFFER>
    {
      public:
        /// @brief Combines all structs used for initialization
        struct CreateInfo
        {
            /// @brief Buffer Create info used to configure Vulkan buffer create
            VkBufferCreateInfo BufferCreateInfo{};
            /// @brief Allocation create info used to configure Vma
            VmaAllocationCreateInfo AllocationCreateInfo{};
            /// @brief If non zero, Vma's aligned buffer allocate function is invoked
            VkDeviceSize Alignment{};
            /// @brief Debug name assigned to both VmaAllocation and VkBuffer vulkan object
            std::string Name{};

            /// @brief Default constructor (initializes .sType field and nulls everything else)
            CreateInfo();
            /// @brief Shorthand for most commonly used fields
            CreateInfo(VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = {}, std::string_view name = "");
        };

        static CreateInfo CreateForStaging(VkDeviceSize size, std::string_view bufferName = {});
      public:
        ManagedBuffer(Context* context, const CreateInfo& createInfo);
        ManagedBuffer(Context* context, VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = {}, std::string_view name = "");

        /// @brief Employ a staging buffer to upload data
        /// @param data data
        /// @param size size
        /// @param offset write offset into buffer
        void WriteDataDeviceLocal(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
        /// @brief Create staging buffer, copy data to it, copy data to device buffer, destroy staging buffer
        /// @param cmdBuffer Single use command buffer. Must be completely reset, function will automatically begin, write, submit and resynchronize.
        /// @param data data
        /// @param size size
        /// @param offset write offset into buffer
        void WriteDataDeviceLocal(HostSyncCommandBuffer& cmdBuffer, const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        /// @brief Maps the buffer to memory address data
        void Map(void*& data);
        /// @brief Unmaps the buffer
        void Unmap();

        /// @brief Attempts to map buffer, write data, unmap
        /// @param size Amount to write. If zero, writes equal to the entire size of the buffer
        void MapAndWrite(const void* data, size_t size = 0);

        virtual ~ManagedBuffer();

        FORAY_GETTER_V(Buffer);
        FORAY_GETTER_V(IsMapped);
        FORAY_GETTER_V(Allocation);
        FORAY_GETTER_CR(AllocationInfo);
        FORAY_GETTER_V(Name);
        FORAY_GETTER_V(Size);
        FORAY_GETTER_V(Alignment);

        /// @brief Gets the buffers device address
        VkDeviceAddress GetDeviceAddress() const;

        /// @brief Sets the buffers name. Decorates VkBuffer, VmaAllocation and VulkanResource base class.
        virtual void SetName(std::string_view name) override;

        /// @brief Fills VkDescriptorBufferInfo object with zero offset and full buffer size
        inline VkDescriptorBufferInfo GetVkDescriptorBufferInfo() const { return VkDescriptorBufferInfo{.buffer = mBuffer, .offset = 0, .range = mSize}; }
        /// @brief Same as GetVkDescriptorBufferInfo, but writing to an existing object
        void FillVkDescriptorBufferInfo(VkDescriptorBufferInfo& bufferInfo) const;

      protected:
        Context*          mContext{};
        VkBuffer          mBuffer{};
        VmaAllocation     mAllocation{};
        VmaAllocationInfo mAllocationInfo{};
        VkDeviceSize      mSize      = {};
        VkDeviceSize      mAlignment = {};
        bool              mIsMapped  = false;

        void UpdateDebugNames();
    };

}  // namespace foray::core