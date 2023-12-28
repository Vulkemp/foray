#pragma once
#include "../basics.hpp"
#include "../vulkan.hpp"

namespace foray::rtpipe {

    /// @brief Shader types used in Raytracing
    enum class RtShaderType
    {
        Undefined,
        Raygen,
        ClosestHit,
        Anyhit,
        Intersect,
        Miss,
        Callable
    };

    /// @brief ShaderGroup Types
    enum class RtShaderGroupType
    {
        Undefined,
        Raygen,
        Miss,
        Callable,
        /// @brief Hit group contains ClosestHit, AnyHit and Intersect Shader Types
        Hit,
    };

    /// @brief Enum conversion methods
    class RtShaderEnumConversions
    {
      public:
        /// @brief Convert shaderType to vk::ShaderStageFlagBits stage. RtShaderType::Undefined causes Exception!
        static vk::ShaderStageFlagBits ToStage(RtShaderType shaderType);
        /// @brief Get correct group type for shaderType. RtShaderType::Undefined maps to RtShaderGroupType::Undefined
        static RtShaderGroupType     ToGroupType(RtShaderType shaderType);
        /// @brief Get correct group type for vk::ShaderStageFlagBits stage. Non raytracing shaderstages map to RtShaderGroupType::Undefined
        static RtShaderGroupType     ToGroupType(vk::ShaderStageFlagBits stage);
        /// @brief Get shaderType for vk::ShaderStageFlagBits stage. Non raytracing shaderstages map to RtShaderType::Undefined
        static RtShaderType          ToType(vk::ShaderStageFlagBits stage);
    };

    /// @brief Type used for indexing ShaderGroups in SBTs
    using GroupIndex = int32_t;
}  // namespace foray
