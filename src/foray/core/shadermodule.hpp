#pragma once
#include "../vulkan.hpp"
#include "../osi/env.hpp"
#include "context.hpp"
#include "managedresource.hpp"
#include "shadermanager.hpp"
#include <span>

namespace foray::core {
    /// @brief Wraps shader code driver handle (VkShaderModule). See ShaderManager for compiling shaders dynamically.
    class ShaderModule : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE>
    {
      public:
        ShaderModule(Context* context, const osi::Utf8Path& spirvFilePath);
        ShaderModule(Context* context, const uint32_t* spirvBinary, size_t sizeInBytes);
        ShaderModule(Context* context, std::span<const uint32_t> spirvBinary);

        virtual ~ShaderModule();

        template <size_t ARR_SIZE>
        inline ShaderModule(Context* context, const uint8_t binaryBuffer[ARR_SIZE]);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        /// @tparam ARR_SIZE Array size
        template <size_t ARR_SIZE>
        inline ShaderModule(Context* context, const uint32_t binaryBuffer[ARR_SIZE]);

        /// @brief Fill a shader stage create info. .sType, .stage, .module, .pName fields are set, remainder default initialized
        VkPipelineShaderStageCreateInfo GetShaderStageCi(VkShaderStageFlagBits stage, const char* entry = "main") const;

        operator VkShaderModule() const;

      protected:
        Context*       mContext           = nullptr;
        VkShaderModule mShaderModule      = nullptr;
        uint64_t       mShaderCompilerKey = 0;
    };

    template <size_t ARR_SIZE>
    ShaderModule::ShaderModule(Context* context, const uint32_t binaryBuffer[ARR_SIZE])
     : ShaderModule(context, binaryBuffer, ARR_SIZE * sizeof(uint32_t))
    {
    }
}  // namespace foray::core