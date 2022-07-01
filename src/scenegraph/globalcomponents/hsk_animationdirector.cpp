#include "hsk_animationdirector.hpp"

namespace hsk {
    void AnimationDirector::Update(const FrameUpdateInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {
            FrameUpdateInfo animationUpdateInfo(updateInfo);
            animationUpdateInfo.GetFrameTime() *= mPlaybackConfig.PlaybackSpeed;

            for(auto& animation : mAnimations)
            {
                animation.Update(animationUpdateInfo);
            }
        }
    }
}  // namespace hsk