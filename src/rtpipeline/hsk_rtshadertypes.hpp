#pragma once
#include "../hsk_basics.hpp"

namespace hsk {
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

    enum class RtShaderGroupType
    {
        Undefined,
        Raygen,
        Miss,
        Callable,
        Intersect,
    };

    class RtShaderEnumConversions
    {
      public:
        static VkShaderStageFlagBits ToStage(RtShaderType shaderType);
        static RtShaderGroupType     ToGroupType(RtShaderType shaderType);
        static RtShaderGroupType     ToGroupType(VkShaderStageFlagBits stage);
        static RtShaderType          ToType(VkShaderStageFlagBits stage);
    };

    using ShaderHandle  = uint64_t;  // According to vulkan.gpuinfo.org, as of 2022-09, all GPUs use 32 bit values for shader handles. This should be ample for future.
    using SbtBindId     = int32_t;
    using ShaderGroupId = int32_t;
}  // namespace hsk
