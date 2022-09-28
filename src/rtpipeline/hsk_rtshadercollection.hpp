#pragma once
#include "../utility/hsk_shadermodule.hpp"
#include "hsk_rtshadertypes.hpp"

namespace hsk {

    /// @brief Helper class to manage unique Rt ShaderModules
    class RtShaderCollection
    {
      public:
        /// @brief Add a shadermodule. If a new module, also adds an entry to the ShaderStageCi vector
        /// @param module Module
        /// @param type Shader Type
        void Add(ShaderModule* module, RtShaderType type, const char* entryPointName = "main");
        /// @brief Reset
        void Clear();
        /// @brief Get index of a module in the ShaderStageCi vector
        uint32_t IndexOf(ShaderModule* module) const;

        HSK_PROPERTY_CGET(ShaderStageCis)

      protected:
        struct Entry
        {
            ShaderModule* Module         = nullptr;
            RtShaderType  Type           = RtShaderType::Undefined;
            const char*   EntryPointName = nullptr;
            uint32_t      StageCiIndex   = VK_SHADER_UNUSED_KHR;
        };

        std::unordered_map<ShaderModule*, Entry>     mEntries;
        std::vector<VkPipelineShaderStageCreateInfo> mShaderStageCis;
    };
}  // namespace hsk
