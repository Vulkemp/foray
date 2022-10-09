#pragma once
#include "../../foray_glm.hpp"
#include "../foray_animation.hpp"
#include "../foray_component.hpp"
#include <vector>

namespace foray::scene {

    class AnimationDirector : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        FORAY_PROPERTY_ALLGET(Animations)
        FORAY_PROPERTY_ALL(PlaybackConfig)

        virtual void Update(const base::FrameUpdateInfo&) override;

      protected:
        std::vector<Animation> mAnimations;
        PlaybackConfig         mPlaybackConfig;
    };
}  // namespace foray