#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    class ShaderModule
    {
      public:
        ShaderModule() = default;
        ShaderModule(const VkContext* context, std::string relativeSpirvPath);

        ~ShaderModule() { DeleteVkModule(); }

        void LoadFromSpirv(const VkContext* context, std::string relativeShaderPath);
        void DeleteVkModule();

        // A conversion function which allows this ShaderModule to be used
        // in places where VkShaderModule would have been used.
        operator VkShaderModule() const;

      protected:
        const VkContext* mContext;
        std::string mAbsoluteSpirvPath;
        VkShaderModule mShaderModule;
    };
}  // namespace hsk