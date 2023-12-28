#include "rtshadertypes.hpp"

namespace foray::rtpipe {
    vk::ShaderStageFlagBits RtShaderEnumConversions::ToStage(RtShaderType shaderType)
    {
        switch(shaderType)
        {
            case RtShaderType::Raygen:
                return vk::ShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case RtShaderType::ClosestHit:
                return vk::ShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case RtShaderType::Anyhit:
                return vk::ShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case RtShaderType::Intersect:
                return vk::ShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case RtShaderType::Miss:
                return vk::ShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
            case RtShaderType::Callable:
                return vk::ShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            case RtShaderType::Undefined:
            default:
                Exception::Throw("Unable to convert to vk::ShaderStageFlagBits value");
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
    RtShaderGroupType RtShaderEnumConversions::ToGroupType(vk::ShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                return RtShaderGroupType::Raygen;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return RtShaderGroupType::Hit;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
                return RtShaderGroupType::Miss;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                return RtShaderGroupType::Callable;
            default:
                return RtShaderGroupType::Undefined;
        }
    }
    RtShaderType RtShaderEnumConversions::ToType(vk::ShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                return RtShaderType::Raygen;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
                return RtShaderType::ClosestHit;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
                return RtShaderType::Anyhit;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return RtShaderType::Intersect;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR:
                return RtShaderType::Miss;
            case vk::ShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                return RtShaderType::Callable;
            default:
                return RtShaderType::Undefined;
        }
    }
}  // namespace foray
