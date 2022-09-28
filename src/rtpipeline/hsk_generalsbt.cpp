#include "hsk_generalsbt.hpp"
#include "hsk_rtshadercollection.hpp"

namespace hsk {
    GeneralShaderBindingTable::GeneralShaderBindingTable(RtShaderGroupType groupType, VkDeviceSize entryDataSize = 0)
        : ShaderBindingTableBase(entryDataSize), mShaderGroupType(groupType)
    {
    }

    void GeneralShaderBindingTable::SetGroup(GroupIndex groupIndex, ShaderModule* shader)
    {
        Assert(groupIndex >= 0, "ShaderGroup index must be >= 0");
        SetGroup(groupIndex, shader, nullptr);
    }
    void GeneralShaderBindingTable::SetGroup(GroupIndex groupIndex, ShaderModule* shader, const void* data)
    {
        Assert(groupIndex >= 0, "ShaderGroup index must be >= 0");
        Assert(data == nullptr || mEntryDataSize > 0, "Set data size before passing data to groups!");
        if(groupIndex >= mGroups.size())
        {
            mGroups.resize(size_t{groupIndex + 1});
            ArrayResized(mGroups.size());
        }
        if(mEntryDataSize > 0)
        {
            SetData(groupIndex, data);
        }
        mGroups[groupIndex] = ShaderGroup{shader, groupIndex};
    }

    ShaderBindingTableBase::VectorRange GeneralShaderBindingTable::WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const
    {
        VectorRange range{groupCis.size(), 0};
        for(const ShaderGroup& group : mGroups)
        {
            groupCis.push_back(VkRayTracingShaderGroupCreateInfoKHR{
                .sType                           = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .pNext                           = nullptr,
                .type                            = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader                   = shaderCollection.IndexOf(group.Module),
                .closestHitShader                = VK_SHADER_UNUSED_KHR,
                .anyHitShader                    = VK_SHADER_UNUSED_KHR,
                .intersectionShader              = VK_SHADER_UNUSED_KHR,
                .pShaderGroupCaptureReplayHandle = nullptr,
            });
        }
        range.Count = int32_t{groupCis.size()} - range.Start;
        return range;
    }

    void GeneralShaderBindingTable::WriteToShaderCollection(RtShaderCollection& collection) const
    {
        RtShaderType shaderType;
        switch(mShaderGroupType)
        {
            case RtShaderGroupType::Raygen:
                shaderType = RtShaderType::Raygen;
                break;
            case RtShaderGroupType::Callable:
                shaderType = RtShaderType::Callable;
                break;
            case RtShaderGroupType::Miss:
                shaderType = RtShaderType::Miss;
                break;
            default:
                Exception::Throw("GeneralShaderBindingTable must be initialized to Raygen, Miss or Callable group type!");
                break;
        }
        for(const ShaderGroup& shader : mGroups)
        {
            if(shader.Module == nullptr || shader.Index < 0)
            {
                continue;
            }
            collection.Add(shader.Module, shaderType);
        }
    }
}  // namespace hsk
