#pragma once
#include "../../glm.hpp"
#include "../animation.hpp"
#include "../component.hpp"
#include <vector>

namespace foray::scene::gcomp {

    /// @brief Handles storage and playback of animations
    class AnimationManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        inline AnimationManager() : Component::UpdateCallback(0) {}

        FORAY_PROPERTY_R(Animations)
        FORAY_PROPERTY_R(PlaybackConfig)

        virtual void Update(SceneUpdateInfo&) override;

      protected:
        std::vector<Animation> mAnimations;
        PlaybackConfig         mPlaybackConfig;
    };
}  // namespace foray