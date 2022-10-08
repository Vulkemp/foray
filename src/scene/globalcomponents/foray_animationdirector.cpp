#include "foray_animationdirector.hpp"

namespace foray::scene {
    void AnimationDirector::Update(const base::FrameUpdateInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {
            base::FrameUpdateInfo animationUpdateInfo(updateInfo);
            animationUpdateInfo.GetFrameTime() *= mPlaybackConfig.PlaybackSpeed;

            for(auto& animation : mAnimations)
            {
                animation.Update(animationUpdateInfo);
            }
        }
    }
}  // namespace foray