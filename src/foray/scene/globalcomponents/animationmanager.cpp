#include "animationmanager.hpp"

namespace foray::scene::gcomp {
    void AnimationManager::Update(SceneUpdateInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {
            base::FrameRenderInfo animationUpdateInfo(updateInfo.RenderInfo);
            animationUpdateInfo.SetFrameTime(animationUpdateInfo.GetFrameTime() * mPlaybackConfig.PlaybackSpeed);

            for(auto& animation : mAnimations)
            {
                animation.Update(animationUpdateInfo);
            }
        }
    }
}  // namespace foray