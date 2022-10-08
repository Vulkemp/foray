#pragma once
#include "../foray_glm.hpp"

namespace foray::scene {
    /// @brief Describes a simplified light source
    struct SimplifiedLight
    {
        /// @brief Radiant flux in Watt per primary wavelength
        alignas(16) glm::vec3 RadiantFluxRgb;
        /// @brief Type (see SimplifiedLightType enum values)
        uint32_t Type;
        /// @brief Position or direction, interprete based on type
        alignas(16) glm::vec3 PosOrDir;
    };
}  // namespace foray