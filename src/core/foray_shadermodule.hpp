#pragma once
#include "../foray_vulkan.hpp"
#include "../osi/foray_env.hpp"
#include "foray_context.hpp"
#include "foray_managedresource.hpp"
#include "foray_shadermanager.hpp"

namespace foray::core {
    /// @brief Wraps shader code driver handle (VkShaderModule). See ShaderManager for compiling shaders dynamically.
    class ShaderModule : public core::VulkanResource<VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE>
    {
      public:
        ShaderModule() = default;

        inline ~ShaderModule() { Destroy(); }

        /// @brief Loads by compiling from source using the ShaderManager
        /// @param context Requires ShaderMan, DispatchTable
        /// @param path Path of the source file (glsl or hlsl shader source)
        /// @param config Optional struct for configuring the shader compiler
        /// @return The shader compilation key
        uint64_t CompileFromSource(Context* context, const osi::Utf8Path& path, const ShaderCompilerConfig& config = {});
        /// @brief Loads from a spirv file
        /// @param context Requires DispatchTable
        /// @param path Spirv file path (absolute or relative to current working directory)
        void LoadFromFile(Context* context, const osi::Utf8Path& path);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        inline void LoadFromBinary(Context* context, const std::vector<uint8_t>& binaryBuffer);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        inline void LoadFromBinary(Context* context, const std::vector<uint32_t>& binaryBuffer);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        /// @tparam ARR_SIZE Array size
        template <size_t ARR_SIZE>
        inline void LoadFromBinary(Context* context, const uint8_t binaryBuffer[ARR_SIZE]);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        /// @tparam ARR_SIZE Array size
        template <size_t ARR_SIZE>
        inline void LoadFromBinary(Context* context, const uint32_t binaryBuffer[ARR_SIZE]);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        /// @param sizeInBytes Size in bytes
        void LoadFromBinary(Context* context, const uint8_t* binaryBuffer, size_t sizeInBytes);
        /// @brief Loads from a binary buffer
        /// @param context Requires DispatchTable
        /// @param binaryBuffer Binary data
        /// @param sizeInBytes Size in bytes
        void LoadFromBinary(Context* context, const uint32_t* binaryBuffer, size_t sizeInBytes);

        inline virtual bool Exists() const override { return !!mShaderModule; }

        virtual void Destroy() override;

        /// @brief Fill a shader stage create info. .sType, .stage, .module, .pName fields are set, remainder default initialized
        VkPipelineShaderStageCreateInfo GetShaderStageCi(VkShaderStageFlagBits stage, const char* entry = "main") const;

        operator VkShaderModule() const;

      protected:
        Context*       mContext      = nullptr;
        VkShaderModule mShaderModule = nullptr;
    };

    void ShaderModule::LoadFromBinary(Context* context, const std::vector<uint8_t>& binaryBuffer)
    {
        return LoadFromBinary(context, binaryBuffer.data(), binaryBuffer.size());
    }

    void ShaderModule::LoadFromBinary(Context* context, const std::vector<uint32_t>& binaryBuffer)
    {
        return LoadFromBinary(context, binaryBuffer.data(), binaryBuffer.size());
    }

    template <size_t ARR_SIZE>
    void ShaderModule::LoadFromBinary(Context* context, const uint8_t binaryBuffer[ARR_SIZE])
    {
        return LoadFromBinary(context, binaryBuffer, ARR_SIZE);
    }

    template <size_t ARR_SIZE>
    void ShaderModule::LoadFromBinary(Context* context, const uint32_t binaryBuffer[ARR_SIZE])
    {
        return LoadFromBinary(context, binaryBuffer, ARR_SIZE * sizeof(uint32_t));
    }
}  // namespace foray::core