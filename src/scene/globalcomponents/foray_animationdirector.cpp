#include "foray_animationdirector.hpp"

namespace foray::scene {
    void AnimationDirector::Update(SceneUpdateInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {
            base::FrameRenderInfo animationUpdateInfo(updateInfo.RenderInfo);
            animationUpdateInfo.GetFrameTime() *= mPlaybackConfig.PlaybackSpeed;

            for(auto& animation : mAnimations)
            {
                animation.Update(animationUpdateInfo);
            }
        }
    }
}  // namespace foray