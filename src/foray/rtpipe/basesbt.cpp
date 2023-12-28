#include "basesbt.hpp"

namespace foray::rtpipe {

    ShaderBindingTableBase::Builder& ShaderBindingTableBase::Builder::SetEntryData(int32_t groupIdx, const void* data, std::size_t size)
    {
        Assert(groupIdx >= 0);
        if(groupIdx >= (int32_t)mGroupData.size())
        {
            mGroupData.resize(groupIdx + 1);
        }
        if(size > mEntryDataSize)
        {
            mEntryDataSize = size;
        }
        mGroupData[groupIdx].resize(size);
        memcpy(mGroupData[groupIdx].data(), data, size);
        return *this;
    }

    std::span<const uint8_t> ShaderBindingTableBase::Builder::GetEntryData(int32_t groupIdx) const
    {
        Assert(groupIdx >= 0 && groupIdx < (int32_t)mGroupData.size(), "Group Index out of range");
        return std::span{mGroupData[groupIdx].data(), mGroupData[groupIdx].size()};
    }

    // std::span<uint8_t> ShaderBindingTableBase::GetEntryData(int32_t groupIdx)
    // {
    //     Assert(groupIdx >= 0 && groupIdx < (int32_t)mConfig.GroupData.size(), "Group Index out of range");
    //     return std::span{mConfig.GroupData[groupIdx].data(), mConfig.GroupData[groupIdx].size()};
    // }

    ShaderBindingTableBase::ShaderBindingTableBase(core::Context* context, const Builder& builder)
    {
        /// STEP # 0    Calculate entry size

        VkDeviceSize entryCount = builder.GetGroupData().size();


        // Calculate size of individual entries. These always consist of a shader handle, followed by optional shader data. Alignment rules have to be observed.
        const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties = *builder.GetPipelineProperties();
        VkDeviceSize                                           sbtEntrySize       = pipelineProperties.shaderGroupHandleSize + builder.GetEntryDataSize();
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

        const vk::BufferUsageFlags sbtBufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VmaMemoryUsage     sbtMemoryFlags      = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        const VmaAllocationCreateFlags sbtAllocFlags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        core::ManagedBuffer::CreateInfo ci(sbtBufferUsageFlags, bufferSize, sbtMemoryFlags, sbtAllocFlags);
        ci.Alignment = bufferAlignment;

        if(bufferSize > 0)
        {

            mBuffer.New(context, ci);
            mAddressRegion = VkStridedDeviceAddressRegionKHR{
                .deviceAddress = mBuffer->GetDeviceAddress(),
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

        for(int32_t i = 0; i < (int32_t)entryCount; i++)
        {
            uint8_t*       bufferEntry = bufferData.data() + (i * sbtEntrySize);
            const uint8_t* handle      = (*builder.GetSgHandles())[i];
            memcpy(bufferEntry, handle, (size_t)pipelineProperties.shaderGroupHandleSize);

            // (Optional) copy custom shader data to entry

            if(builder.GetEntryDataSize() > 0)
            {
                bufferEntry = bufferEntry + pipelineProperties.shaderGroupHandleSize;
                memset(bufferEntry, 0, builder.GetEntryDataSize());
                std::span<const uint8_t> data = builder.GetEntryData(i);
                if(data.size() > 0)
                {
                    memcpy(bufferEntry, data.data(), data.size());
                }
            }
        }


        /// STEP # 3    Write buffer data

        mBuffer->MapAndWrite(bufferData.data());
    }
}  // namespace foray::rtpipe
