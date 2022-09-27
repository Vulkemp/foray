#include "hsk_rtshadercollection.hpp"

namespace hsk {
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
}  // namespace hsk
