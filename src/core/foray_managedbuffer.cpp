#include "foray_managedbuffer.hpp"
#include "../foray_logger.hpp"
#include "../util/foray_fmtutilities.hpp"
#include "foray_commandbuffer.hpp"
#include <spdlog/fmt/fmt.h>

namespace foray::core {

    ManagedBuffer::CreateInfo::CreateInfo() : BufferCreateInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO} {}

    ManagedBuffer::CreateInfo::CreateInfo(VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags, std::string_view name)
        : BufferCreateInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = usage}, AllocationCreateInfo{.flags = flags, .usage = memoryUsage}, Name(name)
    {
    }

    ManagedBuffer::ManagedBuffer(Context* context, const CreateInfo& createInfo)
     : VulkanResource(createInfo.Name.size() > 0 ? createInfo.Name : "Unnamed Buffer")
    {
        mContext   = context;
        mSize      = createInfo.BufferCreateInfo.size;
        mAlignment = createInfo.Alignment;
        if(mAlignment > 1)
        {
            AssertVkResult(vmaCreateBufferWithAlignment(mContext->Allocator, &createInfo.BufferCreateInfo, &createInfo.AllocationCreateInfo, mAlignment, &mBuffer, &mAllocation,
                                                        &mAllocationInfo));
        }
        else
        {
            AssertVkResult(vmaCreateBuffer(mContext->Allocator, &createInfo.BufferCreateInfo, &createInfo.AllocationCreateInfo, &mBuffer, &mAllocation, &mAllocationInfo));
        }
        if(createInfo.Name.size())
        {
            mName = createInfo.Name;
        }
#ifdef FORAY_DEBUG
        if(mName.length() > 0)
        {
            UpdateDebugNames();
            logger()->debug("ManagedBuffer: Create \"{0}\" Mem {1:x} Buffer {2:x}", mName, reinterpret_cast<uint64_t>(mAllocationInfo.deviceMemory),
                            reinterpret_cast<uint64_t>(mBuffer));
        }
#endif
    }

    ManagedBuffer::CreateInfo ManagedBuffer::CreateForStaging(VkDeviceSize size, std::string_view bufferName)
    {
        return CreateInfo(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
               VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    ManagedBuffer::ManagedBuffer(Context* context, VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags, std::string_view name)
     : ManagedBuffer(context, CreateInfo(usage, size, memoryUsage, flags, name))
    {
    }

    void ManagedBuffer::Map(void*& data)
    {
        Assert(!!mAllocation);
        AssertVkResult(vmaMapMemory(mContext->Allocator, mAllocation, &data));
        mIsMapped = true;
    }

    void ManagedBuffer::Unmap()
    {
        Assert(!!mAllocation);
        vmaUnmapMemory(mContext->Allocator, mAllocation);
        mIsMapped = false;
    }

    void ManagedBuffer::MapAndWrite(const void* data, size_t size)
    {
        if(size == 0)
        {
            size = (size_t)mAllocationInfo.size;
        }
        void* mappedPtr = nullptr;
        AssertVkResult(vmaMapMemory(mContext->Allocator, mAllocation, &mappedPtr));
        memcpy(mappedPtr, data, size);
        vmaUnmapMemory(mContext->Allocator, mAllocation);
    }

    VkDeviceAddress ManagedBuffer::GetDeviceAddress() const
    {
        VkBufferDeviceAddressInfo addressInfo;
        addressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = mBuffer;
        addressInfo.pNext  = nullptr;
        return vkGetBufferDeviceAddress(mContext->VkDevice(), &addressInfo);
    }

    void ManagedBuffer::SetName(std::string_view name)
    {
        mName = name;
#if FORAY_DEBUG
        if(mAllocation)
        {
            UpdateDebugNames();
        }
#endif
    }

    void ManagedBuffer::FillVkDescriptorBufferInfo(VkDescriptorBufferInfo& bufferInfo) const
    {
        bufferInfo.buffer = mBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = mSize;
    }

    void ManagedBuffer::UpdateDebugNames()
    {
        std::string debugName = fmt::format("ManBuf \"{}\" ({})", mName, util::PrintSize(mSize));
        SetObjectName(mContext, mBuffer, debugName, false);
        vmaSetAllocationName(mContext->Allocator, mAllocation, debugName.c_str());
    }

    ManagedBuffer::~ManagedBuffer()
    {
        if(!!mAllocation && mIsMapped)
        {
            Unmap();
        }
        if(mContext && mContext->Allocator && mAllocation)
        {
            vmaDestroyBuffer(mContext->Allocator, mBuffer, mAllocation);
            if(mName.length() > 0)
            {
                logger()->debug("ManagedBuffer: Destroy \"{0}\" Mem {1:x} Buffer {2:x}", mName, reinterpret_cast<uint64_t>(mAllocationInfo.deviceMemory),
                                reinterpret_cast<uint64_t>(mBuffer));
            }
        }
        mBuffer     = nullptr;
        mAllocation = nullptr;
        mSize       = 0;
    }

    void ManagedBuffer::WriteDataDeviceLocal(const void* data, VkDeviceSize size, VkDeviceSize offsetDstBuffer)
    {
        HostSyncCommandBuffer cmdBuffer(mContext);
        WriteDataDeviceLocal(cmdBuffer, data, size, offsetDstBuffer);
    }
    void ManagedBuffer::WriteDataDeviceLocal(HostSyncCommandBuffer& cmdBuffer, const void* data, VkDeviceSize size, VkDeviceSize offsetDstBuffer)
    {
        Assert(size + offsetDstBuffer <= mAllocationInfo.size, "Attempt to write data to device local buffer failed. Size + offsets needs to fit into buffer allocation!");

        ManagedBuffer stagingBuffer(mContext, CreateForStaging(size, fmt::format("Staging for {}", GetName())));
        stagingBuffer.MapAndWrite(data, size);

        cmdBuffer.Begin();

        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = offsetDstBuffer;
        copy.size      = size;

        vkCmdCopyBuffer(cmdBuffer, stagingBuffer.GetBuffer(), mBuffer, 1, &copy);
        cmdBuffer.SubmitAndWait();
    }
}  // namespace foray::core