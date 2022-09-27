#pragma once
#include "hsk_rtshadertypes.hpp"
#include "../utility/hsk_shadermodule.hpp"

namespace hsk {

    class RtShaderCollection
    {
      public:
        struct Entry
        {
            ShaderModule* Module       = nullptr;
            RtShaderType  Type         = RtShaderType::Undefined;
            uint32_t      StageCiIndex = VK_SHADER_UNUSED_KHR;
        };

        void     Add(ShaderModule* module, RtShaderType type);
        void     Clear();
        uint32_t IndexOf(ShaderModule* module) const;

        void BuildShaderStageCiVector();

        HSK_PROPERTY_CGET(ShaderStageCis)

      protected:
        std::unordered_map<ShaderModule*, Entry>     mEntries;
        std::vector<VkPipelineShaderStageCreateInfo> mShaderStageCis;
    };
}  // namespace hsk
