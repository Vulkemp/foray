#pragma once
#include "../../foray_glm.hpp"
#include "../foray_animation.hpp"
#include "../foray_component.hpp"
#include <vector>

namespace foray::scene::gcomp {

    /// @brief Handles storage and playback of animations
    class AnimationManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        FORAY_PROPERTY_ALLGET(Animations)
        FORAY_PROPERTY_ALL(PlaybackConfig)

        virtual void Update(SceneUpdateInfo&) override;

        virtual int32_t GetOrder() const override { return 0; }

      protected:
        std::vector<Animation> mAnimations;
        PlaybackConfig         mPlaybackConfig;
    };
}  // namespace foray