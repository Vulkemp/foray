#pragma once

#include "../basics.hpp"
#include "../vulkan.hpp"
#include <vector>

namespace foray::util {
    /// @brief Helper to create a simple set of shader stage create infos, that all use main as shader start point.
    /// Usage: Create object, add stages with modules, use local variable as in place vector.
    class ShaderStageCreateInfos
    {
      public:
        ShaderStageCreateInfos()                                         = default;
        ShaderStageCreateInfos& operator=(const ShaderStageCreateInfos&) = delete;
        ShaderStageCreateInfos(const ShaderStageCreateInfos&)            = delete;

        std::vector<VkPipelineShaderStageCreateInfo>* Get() { return &mShaderStageCreateInfos; };

        ShaderStageCreateInfos& Add(VkShaderStageFlagBits flagBits, VkShaderModule shaderModule)
        {
            VkPipelineShaderStageCreateInfo shaderStageInfo{};
            shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage  = flagBits;
            shaderStageInfo.module = shaderModule;
            shaderStageInfo.pName  = "main";
            mShaderStageCreateInfos.push_back(shaderStageInfo);
            return *this;
        }

      protected:
        std::vector<VkPipelineShaderStageCreateInfo> mShaderStageCreateInfos;
    };
}  // namespace foray::util