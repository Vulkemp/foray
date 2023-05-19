#pragma once
#include "../../glm.hpp"
#include "../animation.hpp"
#include "../component.hpp"
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