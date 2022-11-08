#pragma once
#include "../core/foray_shadermodule.hpp"
#include "foray_rtshadertypes.hpp"
#include <unordered_map>
#include <vector>

namespace foray::rtpipe {

    /// @brief Helper class to manage unique Rt ShaderModules
    class RtShaderCollection
    {
      public:
        /// @brief Add a shadermodule. If a new module, also adds an entry to the ShaderStageCi vector
        /// @param module Module
        /// @param type Shader Type
        void Add(core::ShaderModule* module, RtShaderType type, const char* entryPointName = "main");
        /// @brief Reset
        void Clear();
        /// @brief Get index of a module in the ShaderStageCi vector
        uint32_t IndexOf(core::ShaderModule* module) const;

        FORAY_GETTER_R(ShaderStageCis)

      protected:
        struct Entry
        {
            core::ShaderModule* Module         = nullptr;
            RtShaderType        Type           = RtShaderType::Undefined;
            const char*         EntryPointName = nullptr;
            uint32_t            StageCiIndex   = VK_SHADER_UNUSED_KHR;
        };

        std::unordered_map<core::ShaderModule*, Entry> mEntries;
        std::vector<VkPipelineShaderStageCreateInfo>   mShaderStageCis;
    };
}  // namespace foray::rtpipe
