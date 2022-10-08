#include "foray_rtshadercollection.hpp"

namespace foray::rtpipe {
    void RtShaderCollection::Add(core::ShaderModule* module, RtShaderType type, const char* entryPointName)
    {
        const auto iter = mEntries.find(module);
        if(iter != mEntries.cend())
        {
            Assert(iter->second.Type == type, "Shader module inserted with different shader type!");
            Assert(strcmp(iter->second.EntryPointName, entryPointName) == 0, "Shader module inserted with different EntryPointName!");
        }
        else
        {
            uint32_t index   = (uint32_t)mShaderStageCis.size();
            mEntries[module] = Entry{module, type, entryPointName, index};
            mShaderStageCis.push_back(VkPipelineShaderStageCreateInfo{.sType  = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                      .stage  = RtShaderEnumConversions::ToStage(type),
                                                                      .module = *module,
                                                                      .pName  = entryPointName});
        }
    }
    void RtShaderCollection::Clear()
    {
        mEntries.clear();
        mShaderStageCis.clear();
    }

    uint32_t RtShaderCollection::IndexOf(core::ShaderModule* module) const
    {
        const auto iter = mEntries.find(module);
        Assert(iter != mEntries.cend(), "Shader module not in collection!");
        const Entry& entry = iter->second;
        return entry.StageCiIndex;
    }
}  // namespace foray::rtpipe
