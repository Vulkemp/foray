#pragma once
#include "../hsk_glm.hpp"

namespace hsk {
    /// @brief Describes a simplified light source
    struct SimplifiedLight
    {
        /// @brief Radiant flux in Watt per primary wavelength
        alignas(16) glm::vec3 RadiantFluxRgb;
        /// @brief Type (see SimplifiedLightType enum values)
        uint Type;
        /// @brief Position or direction, interprete based on type
        alignas(16) glm::vec3 PosOrDir;
    };
}  // namespace hsk