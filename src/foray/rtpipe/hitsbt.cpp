#include "hitsbt.hpp"
#include "rtshadercollection.hpp"

namespace foray::rtpipe {
    HitShaderBindingTable::Builder& HitShaderBindingTable::Builder::SetEntryModules(int32_t             groupIdx,
                                                                                    core::ShaderModule* closestHit,
                                                                                    core::ShaderModule* anyHit,
                                                                                    core::ShaderModule* intersect)
    {
        Assert(groupIdx >= 0);
        if(groupIdx <= (int32_t)mModules.size())
        {
            mModules.resize(groupIdx + 1);
        }
        mModules[groupIdx] = ShaderGroup{closestHit, anyHit, intersect};
        return *this;
    }

    void HitShaderBindingTable::Builder::WriteToShaderCollection(RtShaderCollection& shaderCollection) const
    {
        for(const ShaderGroup& group : mModules)
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

    ShaderBindingTableBase::VectorRange HitShaderBindingTable::Builder::WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis,
                                                                                                   const RtShaderCollection&                          shaderCollection) const
    {
        VectorRange range{(int32_t)groupCis.size(), 0};
        for(const ShaderGroup& group : mModules)
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

    HitShaderBindingTable::HitShaderBindingTable(core::Context* context, const Builder& builder) : ShaderBindingTableBase(context, builder) {}
}  // namespace foray::rtpipe
