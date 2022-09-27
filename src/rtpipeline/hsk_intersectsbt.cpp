#include "hsk_intersectsbt.hpp"
#include "hsk_rtshadercollection.hpp"

namespace hsk {
    void IntersectShaderBindingTable::SetGroup(int32_t index, ShaderGroupId groupId, ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect)
    {
        Assert(index >= 0, "ShaderGroup index must be >= 0");
        SetGroup(index, groupId, closestHit, anyHit, intersect, nullptr);
    }
    void IntersectShaderBindingTable::SetGroup(int32_t index, ShaderGroupId groupId, ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect, const void* data)
    {
        Assert(index >= 0, "ShaderGroup index must be >= 0");
        Assert(data == nullptr || mEntryDataSize > 0, "Set data size before passing data to groups!");
        if(index >= mGroups.size())
        {
            mGroups.resize(index + 1);
            ArrayResized(mGroups.size());
        }
        if(mEntryDataSize > 0)
        {
            SetData(index, data);
        }
        mGroups[index] = ShaderGroup{closestHit, anyHit, intersect, index, groupId};
    }

    void IntersectShaderBindingTable::Build(const VkContext*                                         context,
                                            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR&   pipelineProperties,
                                            const std::unordered_map<ShaderModule*, const uint8_t*>& handles)
    {
        mBuffer.Destroy();

        /// STEP # 0    Calculate entry size

        VkDeviceSize entryCount = mGroups.size();

        // Calculate size of individual entries. These always consist of a shader handle, followed by optional shader data. Alignment rules have to be observed.
        VkDeviceSize sbtEntrySize = pipelineProperties.shaderGroupHandleSize * 3 + mEntryDataSize;
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

        ManagedBuffer::ManagedBufferCreateInfo ci(sbtBufferUsageFlags, bufferSize, sbtMemoryFlags, sbtAllocFlags);
        ci.Alignment = bufferAlignment;
        mBuffer.Create(context, ci);
        mAddressRegion = VkStridedDeviceAddressRegionKHR{
            .deviceAddress = mBuffer.GetDeviceAddress(),
            .stride        = sbtEntrySize,
            .size          = bufferSize,
        };


        /// STEP # 2    Build buffer data

        std::vector<uint8_t> bufferData(bufferSize);

        for(int32_t i = 0; i < entryCount; i++)
        {
            // Copy shader handle to entry
            const ShaderGroup& shaderGroup = mGroups[i];

            // Skip invalid entries (leaves them nulled in buffer)
            if(shaderGroup.ClosestHitModule == nullptr || shaderGroup.Index < 0)
            {
                continue;
            }

            uint8_t*       bufferEntry = bufferData.data() + (i * sbtEntrySize);
            const uint8_t* handle      = handles.at(shaderGroup.ClosestHitModule);
            memcpy(bufferEntry, handle, (size_t)pipelineProperties.shaderGroupHandleSize);

            // (Optional) copy custom shader data to entry

            if(mEntryDataSize > 0)
            {
                bufferEntry      = bufferEntry + pipelineProperties.shaderGroupHandleSize;
                const void* data = GroupDataAt(i);
                memcpy(bufferEntry, data, mEntryDataSize);
            }
        }


        /// STEP # 3    Write buffer data

        mBuffer.MapAndWrite(bufferData.data());
    }

    void IntersectShaderBindingTable::WriteToShaderCollection(RtShaderCollection& shaderCollection) const
    {
        for(const ShaderGroup& group : mGroups)
        {
            if(!!group.ClosestHitModule)
            {
                shaderCollection.Add(group.ClosestHitModule, RtShaderType::ClosestHit);
            }
            if(!!group.AnyHitModule)
            {
                shaderCollection.Add(group.AnyHitModule, RtShaderType::Anyhit);
            }
            if(!!group.IntersectModule)
            {
                shaderCollection.Add(group.IntersectModule, RtShaderType::Intersect);
            }
        }
    }

    void IntersectShaderBindingTable::WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const
    {
        for(const ShaderGroup& group : mGroups)
        {
            groupCis.push_back(VkRayTracingShaderGroupCreateInfoKHR{
                .sType                           = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .pNext                           = nullptr,
                .type                            = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader                   = VK_SHADER_UNUSED_KHR,
                .closestHitShader                = (!!group.ClosestHitModule) ? shaderCollection.IndexOf(group.ClosestHitModule) : VK_SHADER_UNUSED_KHR,
                .anyHitShader                    = (!!group.AnyHitModule) ? shaderCollection.IndexOf(group.AnyHitModule) : VK_SHADER_UNUSED_KHR,
                .intersectionShader              = (!!group.IntersectModule) ? shaderCollection.IndexOf(group.IntersectModule) : VK_SHADER_UNUSED_KHR,
                .pShaderGroupCaptureReplayHandle = nullptr,
            });
        }
    }
}  // namespace hsk
