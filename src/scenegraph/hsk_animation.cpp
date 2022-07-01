#include "hsk_animation.hpp"
#include "components/hsk_transform.hpp"
#include "hsk_node.hpp"

namespace hsk {

    glm::vec4 AnimationSampler::SampleVec4(float time) const
    {
        if(!Keyframes.size())
        {
            return glm::vec4(1.f);
        }

        if(Keyframes.size() == 1)
        {
            return Keyframes[0].Value;
        }

        int32_t lowerIndex = 0;
        int32_t upperIndex = Keyframes.size() - 1;

        bool done = false;
        while(!done)
        {
            done = true;
            if(lowerIndex + 1 < Keyframes.size() && Keyframes[lowerIndex + 1].Time < time)
            {
                lowerIndex++;
                done = false;
            }
            if(upperIndex - 1 >= 0 && Keyframes[upperIndex - 1].Time > time)
            {
                upperIndex--;
                done = false;
            }
        }

        const auto& lowerKeyFrame = Keyframes[lowerIndex];
        const auto& upperKeyFrame = Keyframes[upperIndex];


        switch(Interpolation)
        {
            case EAnimationInterpolation::Step: {
                return lowerKeyFrame.Value;
            }
            case EAnimationInterpolation::Linear: {
                float dist = upperKeyFrame.Time - lowerKeyFrame.Time;
                float diff = time - lowerKeyFrame.Time;
                float multiUpper = diff / dist;
                float multiLower = 1.f - multiUpper;
                return lowerKeyFrame.Value * multiLower + upperKeyFrame.Value * multiUpper;
            }
            case EAnimationInterpolation::Cubicspline:{
                // TODO Cubic spline interpolation
            }
        }

        return glm::vec4(1.f);
    }

    void Animation::Update(const FrameUpdateInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {
            float delta     = updateInfo.GetFrameTime() * mPlaybackConfig.PlaybackSpeed;
            float newCursor = mPlaybackConfig.Cursor + delta;
            if(newCursor < mStart)
            {
                newCursor = mPlaybackConfig.Loop ? mEnd : mStart;
            }
            else if(newCursor > mEnd)
            {
                newCursor = mPlaybackConfig.Loop ? mStart : mEnd;
            }
            mPlaybackConfig.Cursor = newCursor;
        }
        for(auto& channel : mChannels)
        {
            auto  transform = channel.Target->GetTransform();
            auto& sampler   = mSamplers[channel.SamplerIndex];

            glm::vec4 v = sampler.SampleVec4(mPlaybackConfig.Cursor);

            switch(channel.TargetPath)
            {
                case EAnimationTargetPath::Translation:
                    transform->SetTranslation(glm::vec3(v.x, v.y, v.z));
                    break;
                case EAnimationTargetPath::Rotation:
                    transform->SetRotation(glm::quat(v.x, v.y, v.z, v.w));
                    break;
                case EAnimationTargetPath::Scale:
                    transform->SetScale(glm::vec3(v.x, v.y, v.z));
                    break;
                default:
                    continue;
            }
            transform->RecalculateGlobalMatrix();
        }
    }
}  // namespace hsk