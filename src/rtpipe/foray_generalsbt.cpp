#include "foray_generalsbt.hpp"
#include "foray_rtshadercollection.hpp"

namespace foray::rtpipe {

    GeneralShaderBindingTable::Builder& GeneralShaderBindingTable::Builder::SetEntryModule(int32_t groupIdx, core::ShaderModule* module)
    {
        Assert(groupIdx >= 0);
        if (groupIdx <= (int32_t)mModules.size())
        {
            mModules.resize(groupIdx + 1);
        }
        mModules[groupIdx] = module;
        return *this;
    }

    ShaderBindingTableBase::VectorRange GeneralShaderBindingTable::Builder::WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis,
                                                                                              const RtShaderCollection&                          shaderCollection) const
    {
        VectorRange range{(int32_t)groupCis.size(), 0};
        for(core::ShaderModule* module : mModules)
        {
            groupCis.push_back(VkRayTracingShaderGroupCreateInfoKHR{
                .sType                           = VkStructureType::VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .pNext                           = nullptr,
                .type                            = VkRayTracingShaderGroupTypeKHR::VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader                   = shaderCollection.IndexOf(module),
                .closestHitShader                = VK_SHADER_UNUSED_KHR,
                .anyHitShader                    = VK_SHADER_UNUSED_KHR,
                .intersectionShader              = VK_SHADER_UNUSED_KHR,
                .pShaderGroupCaptureReplayHandle = nullptr,
            });
        }
        range.Count = (int32_t)groupCis.size() - range.Start;
        return range;
    }

    void GeneralShaderBindingTable::Builder::WriteToShaderCollection(RtShaderCollection& collection) const
    {
        RtShaderType shaderType;
        switch(mGroupType)
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
        for(core::ShaderModule* shader : mModules)
        {
            collection.Add(shader, shaderType);
        }
    }

    GeneralShaderBindingTable::GeneralShaderBindingTable(core::Context* context, const Builder& builder)
        : ShaderBindingTableBase(context, builder)
    {
    }
}  // namespace foray::rtpipe
