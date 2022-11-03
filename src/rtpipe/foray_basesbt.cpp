#include "foray_basesbt.hpp"

namespace foray::rtpipe {
    ShaderBindingTableBase::ShaderBindingTableBase(VkDeviceSize entryDataSize) : mEntryDataSize(entryDataSize) {}

    std::vector<uint8_t>& ShaderBindingTableBase::GroupDataAt(int32_t groupIndex)
    {
        Assert(mEntryDataSize != 0, "Entry data size not set!");
        Assert(groupIndex >= 0 && groupIndex < GetGroupArrayCount(), "Group Index out of range");
        return mGroupData[groupIndex];
    }

    const std::vector<uint8_t>& ShaderBindingTableBase::GroupDataAt(GroupIndex groupIndex) const
    {
        Assert(mEntryDataSize != 0, "Entry data size not set!");
        Assert(groupIndex >= 0 && groupIndex < GetGroupArrayCount(), "Group Index out of range");
        return mGroupData[groupIndex];
    }

    void ShaderBindingTableBase::Build(core::Context*                                 context,
                                       const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties,
                                       const std::vector<const uint8_t*>&                     handles)
    {
        mBuffer.Destroy();

        /// STEP # 0    Calculate entry size

        VkDeviceSize entryCount = GetGroupArrayCount();

        // Calculate size of individual entries. These always consist of a shader handle, followed by optional shader data. Alignment rules have to be observed.
        VkDeviceSize sbtEntrySize = pipelineProperties.shaderGroupHandleSize + mEntryDataSize;
        if(pipelineProperties.shaderGroupHandleAlignment > 0)
        {
            // Make sure every entry start is aligned to the shader group handle alignment rule
            sbtEntrySize = ((sbtEntrySize + (pipelineProperties.shaderGroupHandleAlignment - 1)) / pipelineProperties.shaderGroupHandleAlignment)
                           * pipelineProperties.shaderGroupHandleAlignment;
        }
        Assert(sbtEntrySize <= pipelineProperties.maxShaderGroupStride, "Shader Data causes max group stride overflow!");


        /// STEP # 1    Allocate buffer

        // The buffer may need to observe alignment rules
        VkDeviceSize bufferAlignment = std::max(pipelineProperties.shaderGroupBaseAlignment, pipelineProperties.shaderGroupHandleAlignment);
        // Full buffer size
        VkDeviceSize bufferSize = sbtEntrySize * entryCount;

        const VkBufferUsageFlags sbtBufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VmaMemoryUsage     sbtMemoryFlags      = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        const VmaAllocationCreateFlags sbtAllocFlags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        core::ManagedBuffer::CreateInfo ci(sbtBufferUsageFlags, bufferSize, sbtMemoryFlags, sbtAllocFlags);
        ci.Alignment = bufferAlignment;

        if(bufferSize > 0)
        {

            mBuffer.Create(context, ci);
            mAddressRegion = VkStridedDeviceAddressRegionKHR{
                .deviceAddress = mBuffer.GetDeviceAddress(),
                .stride        = sbtEntrySize,
                .size          = bufferSize,
            };
        }
        else
        {
            mAddressRegion = VkStridedDeviceAddressRegionKHR{};
            return;
        }


        /// STEP # 2    Build buffer data

        std::vector<uint8_t> bufferData(bufferSize);

        for(int32_t i = 0; i < entryCount; i++)
        {
            uint8_t*       bufferEntry = bufferData.data() + (i * sbtEntrySize);
            const uint8_t* handle      = handles[i];
            memcpy(bufferEntry, handle, (size_t)pipelineProperties.shaderGroupHandleSize);

            // (Optional) copy custom shader data to entry

            if(mEntryDataSize > 0)
            {
                bufferEntry                      = bufferEntry + pipelineProperties.shaderGroupHandleSize;
                const std::vector<uint8_t>& data = GroupDataAt(i);
                if(data.size() > 0)
                {
                    memcpy(bufferEntry, data.data(), mEntryDataSize);
                }
                else
                {
                    memset(bufferEntry, 0, mEntryDataSize);
                }
            }
        }


        /// STEP # 3    Write buffer data

        mBuffer.MapAndWrite(bufferData.data());
    }

    void ShaderBindingTableBase::SetData(GroupIndex groupIndex, const void* data)
    {
        Assert(groupIndex >= 0 && groupIndex < GetGroupArrayCount(), "Group Index out of range!");
        if(mEntryDataSize == 0 || !data)
        {
            return;
        }
        std::vector<uint8_t>& entry = mGroupData[groupIndex];
        entry.resize(mEntryDataSize);
        memcpy(entry.data(), data, mEntryDataSize);
    }

    void ShaderBindingTableBase::ArrayResized(size_t newSize)
    {
        if(mEntryDataSize == 0)
        {
            return;
        }
        mGroupData.resize(newSize);
    }

    ShaderBindingTableBase& ShaderBindingTableBase::SetEntryDataSize(VkDeviceSize newSize)
    {
        if(newSize == mEntryDataSize)
        {
            return *this;
        }
        if(mEntryDataSize > 0 && GetGroupArrayCount() > 0)
        {
            for(std::vector<uint8_t>& data : mGroupData)
            {
                data.resize(newSize);
            }
        }
        mEntryDataSize = newSize;
        return *this;
    }

    void ShaderBindingTableBase::Destroy()
    {
        mGroupData.clear();
        mEntryDataSize = 0;
        mBuffer.Destroy();
        mAddressRegion = {};
    }
}  // namespace foray::rtpipe
