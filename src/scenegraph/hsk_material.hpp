#pragma once
#include "glm/glm.hpp"
#include "hsk_scenegraph_declares.hpp"

namespace hsk {
    struct alignas(16) NMaterialBufferObject  // 52 Bytes, aligned to 16 bytes causes size to be padded to a total of 64 bytes
    {
        glm::vec4 BaseColorFactor;                // Base Color / Albedo Factor
        glm::vec3 EmissiveFactor;                 // Emissive Factor
        fp32_t    MetallicFactor;                 // Metallic Factor
        fp32_t    RoughnessFactor;                // Roughness Factor
        int32_t   BaseColorTextureIndex;          // Texture Index for BaseColor
        int32_t   MetallicRoughnessTextureIndex;  // Texture Index for MetallicRoughness
        int32_t   EmissiveTextureIndex;           // Texture Index for Emissive
        int32_t   NormalTextureIndex;             // Texture Index for Normal
    };
}  // namespace hsk