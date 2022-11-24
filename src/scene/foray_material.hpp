#pragma once
#include "../foray_glm.hpp"
#include "foray_scene_declares.hpp"

namespace foray::scene {

    enum class MaterialFlagBits : uint32_t
    {
        FullyOpaque = 0b1,
        DoubleSided = 0b10
    };

    using MaterialFlags = uint32_t;

    /// @brief Represents the default gltf pbr material type, capable of representing opaque surfaces with a metallic/roughness model including emission
    struct alignas(16) Material
    {
        //0
        glm::vec4 BaseColorFactor = glm::vec4(0.f, 0.f, 0.f, 1.f);  // Base Color / Albedo Factor
        //16
        glm::vec3 EmissiveFactor = glm::vec3();  // Emissive Factor
        fp32_t    MetallicFactor = 0.f;          // Metallic Factor
        //32
        fp32_t  RoughnessFactor               = 0.f;  // Roughness Factor
        int32_t BaseColorTextureIndex         = -1;   // Texture Index for BaseColor
        int32_t MetallicRoughnessTextureIndex = -1;   // Texture Index for MetallicRoughness
        int32_t EmissiveTextureIndex          = -1;   // Texture Index for Emissive
        //48
        int32_t NormalTextureIndex       = -1;    // Texture Index for Normal
        fp32_t  IndexOfRefraction        = 1.5f;  // Index of Refraction
        fp32_t  TransmissionFactor       = 0.0f;  // Percentage of light transmitted through the surface
        int32_t TransmissionTextureIndex = -1;    // Texture Index for transmission factor (as multiplied with the factor)
        //64
        glm::vec3 AttenuationColor = glm::vec3();  // The color that white light turns into due to absorption when reaching the attenuation distance.
        fp32_t    AttenuationDistance =
            std::numeric_limits<float>::infinity();  // Average distance in the medium that light has to travel before it encounters a particle (world space)
        //80
        MaterialFlags Flags = 0U;
    };
}  // namespace foray