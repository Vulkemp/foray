#pragma once
#include "../glm.hpp"

namespace foray::scene {
    enum class ELightType : uint32_t
    {
        Directional = 0,
        Point       = 1
    };

    /// @brief Describes a simplified light source
    struct alignas(16) SimpleLight
    {
        /// @brief Linear RGB color of emitted light
        alignas(16) glm::vec3 Color;
        /// @brief Type (see SimplifiedLightType enum values)
        ELightType Type;
        /// @brief Position or direction, interprete based on type
        alignas(16) glm::vec3 PosOrDir;
        /// @brief Intensity (candela for point type, lux in directional lights)
        fp32_t Intensity = 1.f;
    };

    /// @brief A reference to an emissive triangle
    struct alignas(16) EmissiveTriangle
    {
        /// @brief Indices into vertex buffer
        uint32_t Indices[3];
        /// @brief Index into a foray::as::GeometryMetaBuffer
        uint32_t MetaBufferIndex;
    };
}  // namespace foray::scene