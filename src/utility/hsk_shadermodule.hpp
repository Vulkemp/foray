#pragma once
#include "../base/hsk_vkcontext.hpp"
#include <string>
#include <vulkan/vulkan.h>

namespace hsk {
    class ShaderModule : public DeviceResourceBase
    {
      public:
        ShaderModule() = default;
        ShaderModule(const VkContext* context, std::string relativeSpirvPath);

        inline ~ShaderModule() { Destroy(); }

        void                LoadFromSpirv(const VkContext* context, std::string relativeShaderPath);
        void                LoadFromBinary(const VkContext* context, std::vector<char>& binaryBuffer);
        inline virtual bool Exists() const override { return !!mShaderModule; }
        virtual void        Destroy() override;

        // A conversion function which allows this ShaderModule to be used
        // in places where VkShaderModule would have been used.
        operator VkShaderModule() const;

      protected:
        const VkContext* mContext           = nullptr;
        std::string      mAbsoluteSpirvPath = "";
        VkShaderModule   mShaderModule      = nullptr;
    };
}  // namespace hsk