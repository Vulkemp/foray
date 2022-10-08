#pragma once
#include "../foray_vulkan.hpp"
#include "foray_deviceresource.hpp"
#include "foray_vkcontext.hpp"

namespace foray::core {
    class ShaderModule : public core::DeviceResourceBase
    {
      public:
        ShaderModule() = default;
        ShaderModule(const core::VkContext* context, std::string relativeSpirvPath);

        inline ~ShaderModule() { Destroy(); }

        void                LoadFromSpirv(const VkContext* context, std::string relativeShaderPath);
        void                LoadFromSource(const VkContext* context, std::string relativeShaderSourcePath);
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
}  // namespace foray::core