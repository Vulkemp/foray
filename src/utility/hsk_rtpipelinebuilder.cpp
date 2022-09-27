#include "hsk_rtpipelinebuilder.hpp"

namespace hsk {
    VkShaderStageFlagBits RtShaderEnumConversions::ToStage(RtShaderType shaderType)
    {
        switch(shaderType)
        {
            case RtShaderType::Raygen:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case RtShaderType::ClosestHit:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case RtShaderType::Anyhit:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case RtShaderType::Intersect:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case RtShaderType::Miss:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
            case RtShaderType::Callable:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            case RtShaderType::Undefined:
            default:
                Exception::Throw("Unable to convert to VkShaderStageFlagBits value");
        }
    }
    RtShaderGroupType RtShaderEnumConversions::ToGroupType(RtShaderType shaderType)
    {
        switch(shaderType)
        {
            case RtShaderType::Raygen:
                return RtShaderGroupType::Raygen;
            case RtShaderType::ClosestHit:
            case RtShaderType::Anyhit:
            case RtShaderType::Intersect:
                return RtShaderGroupType::Intersect;
            case RtShaderType::Miss:
                return RtShaderGroupType::Miss;
            case RtShaderType::Callable:
                return RtShaderGroupType::Callable;
            case RtShaderType::Undefined:
            default:
                return RtShaderGroupType::Undefined;
        }
    }
    RtShaderGroupType RtShaderEnumConversions::ToGroupType(VkShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                return RtShaderGroupType::Raygen;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return RtShaderGroupType::Intersect;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
                return RtShaderGroupType::Miss;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                return RtShaderGroupType::Callable;
            default:
                return RtShaderGroupType::Undefined;
        }
    }
    RtShaderType RtShaderEnumConversions::ToType(VkShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                return RtShaderType::Raygen;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
                return RtShaderType::ClosestHit;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
                return RtShaderType::Anyhit;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return RtShaderType::Intersect;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
                return RtShaderType::Miss;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                return RtShaderType::Callable;
            default:
                return RtShaderType::Undefined;
        }
    }

    ShaderBindingTable& ShaderBindingTable::AddShader(ShaderModule* shader, VkShaderStageFlagBits stage, uint32_t groupIndex)
    {
        if(mShaderDataSize > 0)
        {
            size_t index   = mShaderDataSize * (mShaders.size() + 1);
            size_t newSize = index + mShaderDataSize;
            mShaderData.resize(newSize, 0);
        }
        mShaders.push_back(ShaderReference{shader, stage, groupIndex});
    }
    ShaderBindingTable& ShaderBindingTable::AddShader(ShaderModule* shader, VkShaderStageFlagBits stage, const void* data, uint32_t groupIndex)
    {
        if(mShaderDataSize > 0)
        {
            size_t index   = mShaderDataSize * (mShaders.size() + 1);
            size_t newSize = index + mShaderDataSize;
            mShaderData.resize(newSize);
            if(!!data)
            {
                memcpy(mShaderData.data() + index, data, (size_t)mShaderDataSize);
            }
        }
        mShaders.push_back(ShaderReference{shader, stage, groupIndex});
    }

    void ShaderBindingTable::Build(const VkContext* context, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& pipelineProperties, const std::vector<ShaderHandle>& handles)
    {
        mBuffer.Destroy();

        /// STEP # 0    Verify input data, calculate entry size

        // Make sure handles mem area is the correct size
        VkDeviceSize entryCount = mShaders.size();
        Assert(handles.size() == entryCount, "Count of shader handles does not match count of entries!");

        // Calculate size of individual entries. These always consist of a shader handle, followed by optional shader data. Alignment rules have to be observed.
        VkDeviceSize sbtEntrySize = pipelineProperties.shaderGroupHandleSize + mShaderDataSize;
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
        mDeviceAddressRegion = VkStridedDeviceAddressRegionKHR{
            .deviceAddress = mBuffer.GetDeviceAddress(),
            .stride        = sbtEntrySize,
            .size          = bufferSize,
        };


        /// STEP # 2    Build buffer data

        std::vector<uint8_t> bufferData(bufferSize);

        for(int32_t i = 0; i < entryCount; i++)
        {
            // Copy shader handle to entry

            uint8_t*       bufferEntry = bufferData.data() + (i * sbtEntrySize);
            const uint8_t* handle      = reinterpret_cast<const uint8_t*>(&(handles[i]));
            memcpy(bufferEntry, handle, (size_t)pipelineProperties.shaderGroupHandleSize);

            // (Optional) copy custom shader data to entry

            if(mShaderDataSize > 0)
            {
                bufferEntry         = bufferEntry + pipelineProperties.shaderGroupHandleSize;
                const uint8_t* data = mShaderData.data() + (i * mShaderDataSize);
                memcpy(bufferEntry, data, mShaderDataSize);
            }
        }


        /// STEP # 3    Write buffer data

        mBuffer.MapAndWrite(bufferData.data());
    }

    void ShaderBindingTable::WriteToShaderStageCiVector(std::vector<VkPipelineShaderStageCreateInfo>& out) const
    {
        out.reserve(out.size() + mShaders.size());
        for(const ShaderReference shader : mShaders)
        {
            out.push_back(VkPipelineShaderStageCreateInfo{
                .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = shader.Stage, .module = *(shader.Module), .pName = "main"});
        }
    }

    void ShaderBindingTable::GetShaderGroupIds(std::unordered_set<uint32_t>& groupIds) const
    {
        for(const ShaderReference shader : mShaders)
        {
            groupIds.insert(shader.GroupId);
        }
    }
    void ShaderBindingTable::GetGroupIdIndex(uint32_t groupId, int32_t& first) const
    {
        first = -1;
        for(int32_t i = 0; i < mShaders.size(); i++)
        {
            if(mShaders[i].GroupId == groupId)
            {
                first = i;
                return;
            }
        }
    }
    void ShaderBindingTable::GetGroupIdIndices(uint32_t groupId, int32_t& first, int32_t& count) const
    {
        first = -1;
        count = 0;
        for(int32_t i = 0; i < mShaders.size(); i++)
        {
            if(mShaders[i].GroupId == groupId)
            {
                first = i;
                count = 1;
                for(i++; i < mShaders.size(); i++)
                {
                    if(mShaders[i].GroupId != groupId)
                    {
                        return;
                    }
                    count++;
                }
            }
        }
    }

    void RtPipeline::Build(const VkContext* context)
    {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCis;
        uint32_t                                     intersectOffset = 0;
        uint32_t                                     missOffset      = 0;
        uint32_t                                     callablesOffset = 0;

        mRaygenSbt.WriteToShaderStageCiVector(shaderStageCis);

        intersectOffset = (uint32_t)shaderStageCis.size();
        mIntersectsSbt.WriteToShaderStageCiVector(shaderStageCis);

        missOffset = (uint32_t)shaderStageCis.size();
        mMissSbt.WriteToShaderStageCiVector(shaderStageCis);

        callablesOffset = (uint32_t)shaderStageCis.size();
        mCallablesSbt.WriteToShaderStageCiVector(shaderStageCis);

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

        {  // Raygen Shader Groups
            for(uint32_t i = 0; i < mRaygenSbt.GetShaders().size(); i++)
            {
                const ShaderBindingTable::ShaderReference& shader = mRaygenSbt.GetShaders()[i];

                Assert(shader.Type == RtShaderType::Raygen);

                shaderGroups.push_back(VkRayTracingShaderGroupCreateInfoKHR{.sType              = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                                            .type               = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                                                                            .generalShader      = i,
                                                                            .closestHitShader   = VK_SHADER_UNUSED_KHR,
                                                                            .anyHitShader       = VK_SHADER_UNUSED_KHR,
                                                                            .intersectionShader = VK_SHADER_UNUSED_KHR});
            }
        }
        {  // Intersection + Miss Shader Groups
            std::unordered_set<uint32_t> groupIds;
            mIntersectsSbt.GetShaderGroupIds(groupIds);
            mMissSbt.GetShaderGroupIds(groupIds);

            for(uint32_t groupId : groupIds)
            {
                int32_t miss       = -1;
                int32_t closestHit = -1;
                int32_t anyHit     = -1;
                int32_t intersect  = -1;
                {
                    mMissSbt.GetGroupIdIndex(groupId, miss);
                }
                {
                    int32_t first = -1;
                    int32_t count = 0;
                    mIntersectsSbt.GetGroupIdIndices(groupId, first, count);
                    for(int32_t i = first; i < first + count; i++)
                    {
                        const ShaderBindingTable::ShaderReference& shader = mIntersectsSbt.GetShaders()[i];
                        switch(shader.Type)
                        {
                            case RtShaderType::ClosestHit: {
                                Assert(closestHit == -1, "May only define one closesthit shader per group index!");
                                closestHit = i;
                                break;
                            }
                            case RtShaderType::Anyhit: {
                                Assert(anyHit == -1, "May only define one anyhit shader per group index!");
                                anyHit = i;
                                break;
                            }
                            case RtShaderType::Intersect: {
                                Assert(intersect == -1, "May only define one intersect shader per group index!");
                                intersect = i;
                                break;
                            }
                            default: {
                                Exception::Throw("Unexpected shader type during intersect shader group building!");
                            }
                        }
                    }
                }

                shaderGroups.push_back(
                    VkRayTracingShaderGroupCreateInfoKHR{.sType = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                         .type  = (intersect == -1 ? VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR :
                                                                                     VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR),
                                                         .generalShader      = (miss != -1 ? miss : VK_SHADER_UNUSED_KHR),
                                                         .closestHitShader   = (closestHit != -1 ? closestHit : VK_SHADER_UNUSED_KHR),
                                                         .anyHitShader       = (anyHit != -1 ? anyHit : VK_SHADER_UNUSED_KHR),
                                                         .intersectionShader = (intersect != -1 ? intersect : VK_SHADER_UNUSED_KHR)});
            }

            for(uint32_t i = 0; i < mMissSbt.GetShaders().size(); i++)
            {
                const ShaderBindingTable::ShaderReference& shader = mMissSbt.GetShaders()[i];

                Assert(shader.Type == RtShaderType::Miss);

                shaderGroups.push_back(VkRayTracingShaderGroupCreateInfoKHR{.sType = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                                            .type  = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                                            .generalShader      = i,
                                                                            .closestHitShader   = VK_SHADER_UNUSED_KHR,
                                                                            .anyHitShader       = VK_SHADER_UNUSED_KHR,
                                                                            .intersectionShader = VK_SHADER_UNUSED_KHR});
            }
        }
        {  // Callable Shader Groups
            for(uint32_t i = 0; i < mCallablesSbt.GetShaders().size(); i++)
            {
                const ShaderBindingTable::ShaderReference& shader = mCallablesSbt.GetShaders()[i];

                Assert(shader.Type == RtShaderType::Callable);

                shaderGroups.push_back(VkRayTracingShaderGroupCreateInfoKHR{.sType              = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                                            .type               = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                                                                            .generalShader      = i,
                                                                            .closestHitShader   = VK_SHADER_UNUSED_KHR,
                                                                            .anyHitShader       = VK_SHADER_UNUSED_KHR,
                                                                            .intersectionShader = VK_SHADER_UNUSED_KHR});
            }
        }

        // Create the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{};
        raytracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        raytracingPipelineCreateInfo.stageCount                   = static_cast<uint32_t>(shaderStageCis.size());
        raytracingPipelineCreateInfo.pStages                      = shaderStageCis.data();
        raytracingPipelineCreateInfo.groupCount                   = static_cast<uint32_t>(shaderGroups.size());
        raytracingPipelineCreateInfo.pGroups                      = shaderGroups.data();
        raytracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
        raytracingPipelineCreateInfo.layout                       = mPipelineLayout;

        AssertVkResult(context->DispatchTable.createRayTracingPipelinesKHR(nullptr, nullptr, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline));
    }
}  // namespace hsk
