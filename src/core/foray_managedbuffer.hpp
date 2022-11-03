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

      public:
        ManagedBuffer() : VulkanResource("Unnamed Buffer"){};

        /// @brief Creates the buffer based on createInfo
        /// @param context Requires Allocator, Device, DispatchTable (CommandPool for certain shorthands)
        void Create(Context* context, const CreateInfo& createInfo);
        /// @brief Creates the buffer with VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT. If data is set, also maps writes and unmaps
        /// @param context Requires Allocator, Device, DispatchTable
        void CreateForStaging(Context* context, VkDeviceSize size, const void* data = nullptr, std::string_view bufferName = {});
        /// @brief Simplified version of Create that omits the use of a create info but should be sufficient for many usecases
        /// @param context Requires Allocator, Device, DispatchTable
        void Create(Context* context, VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = {}, std::string_view name = "");

        virtual void Destroy() override;

        virtual bool Exists() const override { return !!mAllocation; }

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
        void WriteDataDeviceLocal(HostCommandBuffer& cmdBuffer, const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        /// @brief Maps the buffer to memory address data
        void Map(void*& data);
        /// @brief Unmaps the buffer
        void Unmap();

        /// @brief Attempts to map buffer, write data, unmap
        /// @param size Amount to write. If zero, writes equal to the entire size of the buffer
        void MapAndWrite(const void* data, size_t size = 0);

        inline virtual ~ManagedBuffer()
        {
            if(!!mAllocation)
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