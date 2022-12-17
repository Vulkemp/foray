#pragma once
#include "../foray_basics.hpp"
#include "../foray_vulkan.hpp"

namespace foray::stages {
    namespace rtbindpoints {
        /// @brief Top Level Acceleration Structure Bind Point
        const uint32_t BIND_TLAS = 0;
        /// @brief Output Storage Image Bind Point
        const uint32_t BIND_OUT_IMAGE = 1;
        /// @brief Camera Ubo Buffer Bind Point
        const uint32_t BIND_CAMERA_UBO = 2;
        /// @brief Vertex Buffer Bind Point
        const uint32_t BIND_VERTICES = 3;
        /// @brief Index Buffer Bind Point
        const uint32_t BIND_INDICES = 4;
        /// @brief Material Buffer Bind Point
        const uint32_t BIND_MATERIAL_BUFFER = 5;
        /// @brief Texture Array Bind Point
        const uint32_t BIND_TEXTURES_ARRAY = 6;
        /// @brief GeometryMeta Buffer Bind Point (provided by as::Tlas, maps Blas instances to Index Buffer Offsets and Materials)
        const uint32_t BIND_GEOMETRYMETA = 7;
        /// @brief Environmentmap Sampler Bind Point
        const uint32_t BIND_ENVMAP_SPHERESAMPLER = 9;
        /// @brief  Noise Texture Storage Image Bind Point
        const uint32_t BIND_NOISETEX = 10;
    }  // namespace rtbindpoints

    /// @brief All shaderstage flags usable in a raytracing pipeline
    inline constexpr VkShaderStageFlags RTSTAGEFLAGS = VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR
                                                       | VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                                                       | VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
}