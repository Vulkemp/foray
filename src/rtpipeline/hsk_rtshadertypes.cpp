#include "hsk_rtshadertypes.hpp"

namespace hsk {
    VkShaderStageFlagBits RtShaderEnumConversions::ToStage(RtShaderType shaderType)
    {
        switch(shaderType)
        {
            case RtShaderType::Raygen:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case RtShaderType::ClosestHit:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case RtShaderType::Anyhit:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case RtShaderType::Intersect:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case RtShaderType::Miss:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
            case RtShaderType::Callable:
                return VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            case RtShaderType::Undefined:
            default:
                Exception::Throw("Unable to convert to VkShaderStageFlagBits value");
        }
    }
    RtShaderGroupType RtShaderEnumConversions::ToGroupType(RtShaderType shaderType)
    {
        switch(shaderType)
        {
            case RtShaderType::Raygen:
                return RtShaderGroupType::Raygen;
            case RtShaderType::ClosestHit:
            case RtShaderType::Anyhit:
            case RtShaderType::Intersect:
                return RtShaderGroupType::Hit;
            case RtShaderType::Miss:
                return RtShaderGroupType::Miss;
            case RtShaderType::Callable:
                return RtShaderGroupType::Callable;
            case RtShaderType::Undefined:
            default:
                return RtShaderGroupType::Undefined;
        }
    }
    RtShaderGroupType RtShaderEnumConversions::ToGroupType(VkShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                return RtShaderGroupType::Raygen;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return RtShaderGroupType::Hit;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
                return RtShaderGroupType::Miss;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                return RtShaderGroupType::Callable;
            default:
                return RtShaderGroupType::Undefined;
        }
    }
    RtShaderType RtShaderEnumConversions::ToType(VkShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                return RtShaderType::Raygen;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
                return RtShaderType::ClosestHit;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
                return RtShaderType::Anyhit;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return RtShaderType::Intersect;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
                return RtShaderType::Miss;
            case VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                return RtShaderType::Callable;
            default:
                return RtShaderType::Undefined;
        }
    }
}  // namespace hsk
