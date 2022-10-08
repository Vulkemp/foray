#include "foray_hitsbt.hpp"
#include "foray_rtshadercollection.hpp"

namespace foray::rtpipe {
    void HitShaderBindingTable::SetGroup(GroupIndex groupIndex, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect)
    {
        SetGroup(groupIndex, closestHit, anyHit, intersect, nullptr);
    }
    void HitShaderBindingTable::SetGroup(GroupIndex groupIndex, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect, const void* data)
    {
        Assert(groupIndex >= 0, "ShaderGroup index must be >= 0");
        Assert(data == nullptr || mEntryDataSize > 0, "Set data size before passing data to groups!");
        if(groupIndex >= mGroups.size())
        {
            mGroups.resize((size_t)(groupIndex + 1));
            ArrayResized(mGroups.size());
        }
        if(mEntryDataSize > 0)
        {
            SetData(groupIndex, data);
        }
        mGroups[groupIndex] = ShaderGroup{closestHit, anyHit, intersect};
    }

    void HitShaderBindingTable::WriteToShaderCollection(RtShaderCollection& shaderCollection) const
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

    ShaderBindingTableBase::VectorRange HitShaderBindingTable::WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis,
                                                                                          const RtShaderCollection&                          shaderCollection) const
    {
        VectorRange range{(int32_t)groupCis.size(), 0};
        for(const ShaderGroup& group : mGroups)
        {
            VkRayTracingShaderGroupTypeKHR groupType = (!!group.IntersectModule) ? VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR :
                                                                                   VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            groupCis.push_back(VkRayTracingShaderGroupCreateInfoKHR{
                .sType                           = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .pNext                           = nullptr,
                .type                            = groupType,
                .generalShader                   = VK_SHADER_UNUSED_KHR,
                .closestHitShader                = (!!group.ClosestHitModule) ? shaderCollection.IndexOf(group.ClosestHitModule) : VK_SHADER_UNUSED_KHR,
                .anyHitShader                    = (!!group.AnyHitModule) ? shaderCollection.IndexOf(group.AnyHitModule) : VK_SHADER_UNUSED_KHR,
                .intersectionShader              = (!!group.IntersectModule) ? shaderCollection.IndexOf(group.IntersectModule) : VK_SHADER_UNUSED_KHR,
                .pShaderGroupCaptureReplayHandle = nullptr,
            });
        }
        range.Count = (int32_t)groupCis.size() - range.Start;
        return range;
    }

    void HitShaderBindingTable::Destroy()
    {
        mGroups.clear();
        ShaderBindingTableBase::Destroy();
    }
}  // namespace foray::rtpipe
