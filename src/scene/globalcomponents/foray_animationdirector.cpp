#include "foray_animationdirector.hpp"

namespace foray::scene {
    void AnimationDirector::Update(const base::FrameRenderInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {
            base::FrameRenderInfo animationUpdateInfo(updateInfo);
            animationUpdateInfo.GetFrameTime() *= mPlaybackConfig.PlaybackSpeed;

            for(auto& animation : mAnimations)
            {
                animation.Update(animationUpdateInfo);
            }
        }
    }
}  // namespace foray