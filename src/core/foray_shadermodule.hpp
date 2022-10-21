#pragma once
#include "../foray_vulkan.hpp"
#include "foray_managedresource.hpp"
#include "foray_context.hpp"

namespace foray::core {
    class ShaderModule : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE>
    {
      public:
        ShaderModule() = default;
        ShaderModule(core::Context* context, std::string relativeSpirvPath);

        inline ~ShaderModule() { Destroy(); }

        void                LoadFromSpirv(Context* context, std::string relativeShaderPath);
        void                LoadFromSource(Context* context, std::string relativeShaderSourcePath);
        void                LoadFromBinary(Context* context, std::vector<char>& binaryBuffer);
        inline virtual bool Exists() const override { return !!mShaderModule; }
        virtual void        Destroy() override;

        // A conversion function which allows this ShaderModule to be used
        // in places where VkShaderModule would have been used.
        operator VkShaderModule() const;

      protected:
        Context* mContext           = nullptr;
        std::string      mAbsoluteSpirvPath = "";
        VkShaderModule   mShaderModule      = nullptr;
    };
}  // namespace foray::core