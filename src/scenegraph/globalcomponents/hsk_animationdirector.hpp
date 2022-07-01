#pragma once
#include "../../hsk_glm.hpp"
#include "../hsk_animation.hpp"
#include "../hsk_component.hpp"

namespace hsk {

    class AnimationDirector : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        HSK_PROPERTY_ALLGET(Animations)
        HSK_PROPERTY_ALL(PlaybackConfig)

        virtual void Update(const FrameUpdateInfo&) override;

      protected:
        std::vector<Animation> mAnimations;
        PlaybackConfig         mPlaybackConfig;
    };
}  // namespace hsk