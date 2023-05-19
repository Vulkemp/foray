#pragma once
#include "../../glm.hpp"
#include "../component.hpp"
#include "../scene_declares.hpp"

namespace foray::scene::ncomp {
    /// @brief Defines a simple punctual light (directional or point)
    class AxisAlignedBoundingBox : public NodeComponent
    {
      public:
        FORAY_GETTER_CR(MinBounds)
        FORAY_GETTER_CR(MaxBounds)
        FORAY_GETTER_V(Volume)

        void CompileAABB();

        void SetAABB(const glm::vec3& minBounds, const glm::vec3& maxBounds);

      protected:
        glm::vec3 mMinBounds;
        glm::vec3 mMaxBounds;
        float     mVolume;
    };
}  // namespace foray::scene::ncomp