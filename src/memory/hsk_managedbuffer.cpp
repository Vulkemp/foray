#include "hsk_managedbuffer.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../utility/hsk_fmtutilities.hpp"
#include "hsk_commandbuffer.hpp"
#include "hsk_vmaHelpers.hpp"
#include <spdlog/fmt/fmt.h>

namespace hsk {

    void ManagedBuffer::Create(const VkContext* context, ManagedBufferCreateInfo& createInfo)
    {
        mContext = context;
        mSize    = createInfo.BufferCreateInfo.size;
        AssertVkResult(vmaCreateBuffer(mContext->Allocator, &createInfo.BufferCreateInfo, &createInfo.AllocationCreateInfo, &mBuffer, &mAllocation, &mAllocationInfo));
        if(createInfo.Name.size())
        {
            mName = createInfo.Name;
        }
        if(mName.length() > 0 && mContext->DebugEnabled)
        {
            UpdateDebugNames();
            logger()->debug("ManagedBuffer: Create \"{0}\" Mem {1:x} Buffer {2:x}", mName, reinterpret_cast<uint64_t>(mAllocationInfo.deviceMemory),
                            reinterpret_cast<uint64_t>(mBuffer));
        }
    }

    void ManagedBuffer::CreateForStaging(const VkContext* context, VkDeviceSize size, const void* data, std::string_view bufferName)
    {
        // if (mName.length() == 0){
        //     mName = fmt::format("Staging Buffer {0:x}", reinterpret_cast<uint64_t>(data));
        // }
        if(bufferName.length())
        {
            SetName(bufferName);
        }
        Create(context, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
               VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        if(data)
        {
            MapAndWrite(data);
        }
    }

    void ManagedBuffer::Create(const VkContext* context, VkBufferUsageFlags usage, VkDeviceSize size, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags)
    {
        ManagedBufferCreateInfo createInfo;
        createInfo.BufferCreateInfo.size      = size;
        createInfo.BufferCreateInfo.usage     = usage;
        createInfo.AllocationCreateInfo.usage = memoryUsage;
        createInfo.AllocationCreateInfo.flags = flags;

        Create(context, createInfo);
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

    VkDeviceAddress ManagedBuffer::GetDeviceAddress()
    {
        VkBufferDeviceAddressInfo addressInfo;
        addressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        addressInfo.buffer = mBuffer;
        addressInfo.pNext  = nullptr;
        return vkGetBufferDeviceAddress(mContext->Device, &addressInfo);
    }

    ManagedBuffer& ManagedBuffer::SetName(std::string_view name)
    {
        mName = name;
        if(mAllocation && mContext->DebugEnabled)
        {
            UpdateDebugNames();
        }
        return *this;
    }

    void ManagedBuffer::FillVkDescriptorBufferInfo(VkDescriptorBufferInfo* bufferInfo)
    {
        bufferInfo->buffer = mBuffer;
        bufferInfo->offset = 0;
        bufferInfo->range  = mSize;
    }

    void ManagedBuffer::UpdateDebugNames()
    {
        std::string debugName = fmt::format("Buffer Managed \"{}\" ({})", mName, PrintSize(mSize));
        vmaSetAllocationName(mContext->Allocator, mAllocation, debugName.c_str());
        VkDebugUtilsObjectNameInfoEXT nameInfo{.sType        = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                               .pNext        = nullptr,
                                               .objectType   = VkObjectType::VK_OBJECT_TYPE_BUFFER,
                                               .objectHandle = reinterpret_cast<uint64_t>(mBuffer),
                                               .pObjectName  = debugName.c_str()};
        mContext->DispatchTable.setDebugUtilsObjectNameEXT(&nameInfo);
    }

    void ManagedBuffer::Cleanup()
    {
        if(mIsMapped)
        {
            logger()->warn("ManagedBuffer::Cleanup called before Unmap!");
            Unmap();
        }
        if(mContext && mContext->Allocator && mAllocation)
        {
            vmaDestroyBuffer(mContext->Allocator, mBuffer, mAllocation);
            if(mName.length() > 0)
            {
                logger()->debug("ManagedBuffer: Cleanup \"{0}\" Mem {1:x} Buffer {2:x}", mName, reinterpret_cast<uint64_t>(mAllocationInfo.deviceMemory),
                                reinterpret_cast<uint64_t>(mBuffer));
            }
        }
        mBuffer     = nullptr;
        mAllocation = nullptr;
        mSize       = 0;
    }

    void ManagedBuffer::WriteDataDeviceLocal(const void* data, VkDeviceSize size, VkDeviceSize offsetDstBuffer)
    {
        Assert(size + offsetDstBuffer <= mAllocationInfo.size, "Attempt to write data to device local buffer failed. Size + offsets needs to fit into buffer allocation!");

        ManagedBuffer stagingBuffer;
        stagingBuffer.CreateForStaging(mContext, size, data, fmt::format("Staging for {}", GetName()));

        CommandBuffer   singleTimeCmdBuf;
        VkCommandBuffer commandBuffer = singleTimeCmdBuf.Create(mContext, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = offsetDstBuffer;
        copy.size      = size;

        vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), mBuffer, 1, &copy);
        singleTimeCmdBuf.Submit();
    }

    ManagedBuffer::ManagedBufferCreateInfo::ManagedBufferCreateInfo()
    {
        BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    }

}  // namespace hsk