#pragma once
#include "../../foray_glm.hpp"
#include "../foray_animation.hpp"
#include "../foray_component.hpp"
#include <vector>

namespace foray::scene::gcomp {

    /// @brief Handles storage and playback of animations
    class AabbManager : public GlobalComponent
    {
      public:
        FORAY_GETTER_CR(MinBounds)
        FORAY_GETTER_CR(MaxBounds)

        void CompileAabbs();

      protected:
        glm::vec3 mMinBounds;
        glm::vec3 mMaxBounds;
        float     mVolume;
    };
}  // namespace foray::scene::gcomp