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

    SbtBindId ShaderBindingTable::AddShader(ShaderModule* shader, RtShaderType type, ShaderGroupId groupId)
    {
        if(mShaderDataSize > 0)
        {
            size_t index   = mShaderDataSize * (mShaders.size() + 1);
            size_t newSize = index + mShaderDataSize;
            mShaderData.resize(newSize, 0);
        }
        SbtBindId bindId = (SbtBindId)mShaders.size();
        mShaders.push_back(ShaderReference{shader, type, bindId, groupId});
    }
    SbtBindId ShaderBindingTable::AddShader(ShaderModule* shader, RtShaderType type, const void* data, ShaderGroupId groupId)
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
        SbtBindId bindId = (SbtBindId)mShaders.size();
        mShaders.push_back(ShaderReference{shader, type, bindId, groupId});
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
            out.push_back(VkPipelineShaderStageCreateInfo{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                          .stage  = RtShaderEnumConversions::ToStage(shader.Type),
                                                          .module = *(shader.Module),
                                                          .pName  = "main"});
        }
    }

    void ShaderBindingTable::WriteToShaderCollection(RtShaderCollection& collection) const
    {
        for(const ShaderReference shader : mShaders)
        {
            collection.Add(shader.Module, shader.Type);
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

    void RtShaderCollection::Add(ShaderModule* module, RtShaderType type)
    {
        const auto iter = mEntries.find(module);
        if(iter != mEntries.cend())
        {
            Assert(iter->second.Type == type, "Shader module inserted with different shader type!");
        }
        else
        {
            mEntries[module] = Entry{module, type};
        }
    }
    void RtShaderCollection::Clear()
    {
        mEntries.clear();
    }

    uint32_t RtShaderCollection::IndexOf(ShaderModule* module) const
    {
        const auto iter = mEntries.find(module);
        Assert(iter != mEntries.cend(), "Shader module not in collection!");
        const Entry& entry = iter->second;
        Assert(entry.StageCiIndex != VK_SHADER_UNUSED_KHR, "Build shader stage ci vector first!");
        return entry.StageCiIndex;
    }

    void RtShaderCollection::BuildShaderStageCiVector()
    {
        mShaderStageCis.clear();
        for(auto& entry : mEntries)
        {
            uint32_t index = mShaderStageCis.size();
            mShaderStageCis.push_back(VkPipelineShaderStageCreateInfo{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                      .stage  = RtShaderEnumConversions::ToStage(entry.second.Type),
                                                                      .module = *(entry.second.Module),
                                                                      .pName  = "main"});
            entry.second.StageCiIndex = index;
        }
    }

    uint32_t RtPipeline::AddShaderGroupRaygen(ShaderModule* module)
    {
        ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
        SbtBindId     index   = mRaygenSbt.AddShader(module, RtShaderType::Raygen, groupId);
        mShaderGroups.push_back(ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Raygen, .General = module});
        return groupId;
    }
    uint32_t RtPipeline::AddShaderGroupCallable(ShaderModule* module)
    {
        ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
        SbtBindId     index   = mCallablesSbt.AddShader(module, RtShaderType::Callable, groupId);
        mShaderGroups.push_back(ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Callable, .General = module});
        return groupId;
    }
    uint32_t RtPipeline::AddShaderGroupMiss(ShaderModule* module)
    {
        ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
        SbtBindId     index   = mMissSbt.AddShader(module, RtShaderType::Miss, groupId);
        mShaderGroups.push_back(ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Miss, .General = module});
        return groupId;
    }
    uint32_t RtPipeline::AddShaderGroupIntersect(ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect)
    {
        ShaderGroupId groupId = (ShaderGroupId)mShaderGroups.size();
        ShaderGroup   group   = ShaderGroup{.Id = groupId, .Type = RtShaderGroupType::Intersect};
        if(!!closestHit)
        {
            mIntersectsSbt.AddShader(closestHit, RtShaderType::ClosestHit, groupId);
            group.ClosestHit = closestHit;
        }
        if(!!anyHit)
        {
            mIntersectsSbt.AddShader(anyHit, RtShaderType::Anyhit, groupId);
            group.AnyHit = anyHit;
        }
        if(!!intersect)
        {
            mIntersectsSbt.AddShader(intersect, RtShaderType::Intersect, groupId);
            group.Intersect = intersect;
        }
        mShaderGroups[groupId] = group;
        return groupId;
    }

    struct ShaderGroupHandles
    {
        ShaderHandle General    = {};
        ShaderHandle ClosestHit = {};
        ShaderHandle AnyHit     = {};
        ShaderHandle Intersect  = {};
    };

    VkRayTracingShaderGroupCreateInfoKHR RtPipeline::ShaderGroup::BuildShaderGroupCi(const RtShaderCollection& shaderCollection) const
    {
        switch(Type)
        {
            case RtShaderGroupType::Raygen:
            case RtShaderGroupType::Callable:
            case RtShaderGroupType::Miss: {
                return VkRayTracingShaderGroupCreateInfoKHR{.sType              = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                            .type               = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                                                            .generalShader      = shaderCollection.IndexOf(General),
                                                            .closestHitShader   = VK_SHADER_UNUSED_KHR,
                                                            .anyHitShader       = VK_SHADER_UNUSED_KHR,
                                                            .intersectionShader = VK_SHADER_UNUSED_KHR};
                break;
            }
            case RtShaderGroupType::Intersect: {
                VkRayTracingShaderGroupTypeKHR type = (!!Intersect) ? VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR :
                                                                      VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                return VkRayTracingShaderGroupCreateInfoKHR{.sType              = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                                                            .type               = type,
                                                            .generalShader      = VK_SHADER_UNUSED_KHR,
                                                            .closestHitShader   = (!!ClosestHit) ? shaderCollection.IndexOf(ClosestHit) : VK_SHADER_UNUSED_KHR,
                                                            .anyHitShader       = (!!AnyHit) ? shaderCollection.IndexOf(AnyHit) : VK_SHADER_UNUSED_KHR,
                                                            .intersectionShader = (!!Intersect) ? shaderCollection.IndexOf(Intersect) : VK_SHADER_UNUSED_KHR};
                break;
            }
            default:
                Exception::Throw("Build ShaderGroupCi for unconfigured shadergroup!");
                break;
        }
    }

    void RtPipeline::Build(const VkContext* context)
    {
        /// STEP # 0    Reset, get physical device properties

        mShaderCollection.Clear();

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        {
            VkPhysicalDeviceProperties2 prop2{};
            prop2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            prop2.pNext = &pipelineProperties;
            vkGetPhysicalDeviceProperties2(context->PhysicalDevice, &prop2);
        }


        /// STEP # 1    Rebuild shader collection

        mRaygenSbt.WriteToShaderCollection(mShaderCollection);
        mMissSbt.WriteToShaderCollection(mShaderCollection);
        mCallablesSbt.WriteToShaderCollection(mShaderCollection);
        mIntersectsSbt.WriteToShaderCollection(mShaderCollection);

        mShaderCollection.BuildShaderStageCiVector();


        /// STEP # 2    Collect ShaderGroup, build sorted vector

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> raygenGroupCis;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> callableGroupCis;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> missGroupCis;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> intersectGroupCis;

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCis;

        for(const ShaderGroup& shaderGroup : mShaderGroups)
        {
            VkRayTracingShaderGroupCreateInfoKHR groupCi = shaderGroup.BuildShaderGroupCi(mShaderCollection);

            switch(shaderGroup.Type)
            {
                case RtShaderGroupType::Raygen: {
                    raygenGroupCis.push_back(groupCi);
                    break;
                }
                case RtShaderGroupType::Miss: {
                    missGroupCis.push_back(groupCi);
                    break;
                }
                case RtShaderGroupType::Callable: {
                    callableGroupCis.push_back(groupCi);
                    break;
                }
                case RtShaderGroupType::Intersect: {
                    intersectGroupCis.push_back(groupCi);
                    break;
                }
            }
        }

        // Insert grouped into shaderGroupCis vector

        shaderGroupCis.reserve(mShaderGroups.size());

        uint32_t missOffset      = 0;
        uint32_t callablesOffset = 0;
        uint32_t intersectOffset = 0;

        shaderGroupCis.insert(shaderGroupCis.end(), raygenGroupCis.begin(), raygenGroupCis.end());
        missOffset = shaderGroupCis.size();
        shaderGroupCis.insert(shaderGroupCis.end(), missGroupCis.begin(), missGroupCis.end());
        callablesOffset = shaderGroupCis.size();
        shaderGroupCis.insert(shaderGroupCis.end(), callableGroupCis.begin(), callableGroupCis.end());
        intersectOffset = shaderGroupCis.size();
        shaderGroupCis.insert(shaderGroupCis.end(), intersectGroupCis.begin(), intersectGroupCis.end());


        /// STEP # 3    Build RT pipeline

        // Create the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{
            .sType                        = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
            .stageCount                   = static_cast<uint32_t>(mShaderCollection.GetShaderStageCis().size()),
            .pStages                      = mShaderCollection.GetShaderStageCis().data(),
            .groupCount                   = static_cast<uint32_t>(shaderGroupCis.size()),
            .pGroups                      = shaderGroupCis.data(),
            .maxPipelineRayRecursionDepth = pipelineProperties.maxRayRecursionDepth,
            .layout                       = mPipelineLayout,
        };

        AssertVkResult(context->DispatchTable.createRayTracingPipelinesKHR(nullptr, nullptr, 1, &raytracingPipelineCreateInfo, nullptr, &mPipeline));


        /// STEP # 4    Get shader handles

        std::vector<uint8_t> shaderHandleData(shaderGroupCis.size() * pipelineProperties.shaderGroupHandleSize);
        AssertVkResult(context->DispatchTable.getRayTracingShaderGroupHandlesKHR(mPipeline, 0, shaderGroupCis.size(), shaderHandleData.size(), shaderHandleData.data()));

        std::vector<ShaderHandle> shaderHandles;
        shaderHandles.reserve(mShaderGroups.size());

        {
            for(const ShaderBindingTable::ShaderReference& shaderRef : mRaygenSbt.GetShaders())
            {
                const uint8_t* dataPtr = shaderHandleData.data() + (shaderRef.GroupId * pipelineProperties.shaderGroupHandleSize);
                ShaderHandle   handle  = {};
                memcpy(&handle, dataPtr, pipelineProperties.shaderGroupHandleSize);
                shaderHandles.push_back(handle);
            }
            mRaygenSbt.Build(context, pipelineProperties, shaderHandles);
        }
        {
            for(const ShaderBindingTable::ShaderReference& shaderRef : mCallablesSbt.GetShaders())
            {
                const uint8_t* dataPtr = shaderHandleData.data() + (shaderRef.GroupId * pipelineProperties.shaderGroupHandleSize);
                ShaderHandle   handle  = {};
                memcpy(&handle, dataPtr, pipelineProperties.shaderGroupHandleSize);
                shaderHandles.push_back(handle);
            }
            mRaygenSbt.Build(context, pipelineProperties, shaderHandles);
        }
    }
}  // namespace hsk
