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

    using GroupIndex = int32_t;
}  // namespace hsk
