#include "hsk_animation.hpp"
#include "components/hsk_transform.hpp"
#include "hsk_node.hpp"

namespace hsk {

    glm::vec3 AnimationSampler::SampleVec(float time) const
    {
        if(!Keyframes.size())
        {
            return glm::vec3();
        }

        if(Keyframes.size() == 1)
        {
            return Keyframes[0].Value;
        }

        AnimationKeyframe lower = {};
        AnimationKeyframe upper = {};
        SelectKeyframe(time, lower, upper);

        switch(Interpolation)
        {
            case EAnimationInterpolation::Step: {
                return InterpolateStep(time, lower, upper);
            }
            case EAnimationInterpolation::Linear: {
                return InterpolateLinear(time, lower, upper);
            }
            case EAnimationInterpolation::Cubicspline: {
                return InterpolateCubicSpline(time, lower, upper);
            }
        }

        return glm::vec3();
    }

    glm::quat AnimationSampler::SampleQuat(float time) const
    {
        if(!Keyframes.size())
        {
            return glm::quat();
        }

        if(Keyframes.size() == 1)
        {
            return ReinterpreteAsQuat(Keyframes[0].Value);
        }

        AnimationKeyframe lower = {};
        AnimationKeyframe upper = {};
        SelectKeyframe(time, lower, upper);

        switch(Interpolation)
        {
            case EAnimationInterpolation::Step: {
                return ReinterpreteAsQuat(InterpolateStep(time, lower, upper));
            }
            case EAnimationInterpolation::Linear: {
                return InterpolateLinearQuat(time, lower, upper);
            }
            case EAnimationInterpolation::Cubicspline: {
                return InterpolateCubicSpline(time, lower, upper);
            }
        }

        return glm::quat();
    }


    void AnimationSampler::SelectKeyframe(float time, AnimationKeyframe& lower, AnimationKeyframe& upper) const
    {
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

        lower = Keyframes[lowerIndex];
        upper = Keyframes[upperIndex];
    }

    glm::vec3 AnimationSampler::InterpolateStep(float time, const AnimationKeyframe& lower, const AnimationKeyframe& upper)
    {
        return lower.Value;
    }

    glm::vec3 AnimationSampler::InterpolateLinear(float time, const AnimationKeyframe& lower, const AnimationKeyframe& upper)
    {
        float dist = upper.Time - lower.Time;
        float diff = time - lower.Time;
        float t    = diff / dist;
        return glm::mix(upper.Value, lower.Value, t);
    }

    glm::quat AnimationSampler::InterpolateLinearQuat(float time, const AnimationKeyframe& lower, const AnimationKeyframe& upper)
    {
        glm::quat lowerQuat = ReinterpreteAsQuat(lower.Value);
        glm::quat upperQuat = ReinterpreteAsQuat(upper.Value);
        float     dist      = upper.Time - lower.Time;
        float     diff      = time - lower.Time;
        float     t         = diff / dist;
        return glm::normalize(glm::slerp(upperQuat, lowerQuat, t));
    }

    glm::vec3 AnimationSampler::InterpolateCubicSpline(float time, const AnimationKeyframe& lower, const AnimationKeyframe& upper)
    {
        // TODO Cubic spline interpolation
        return InterpolateStep(time, lower, upper);
    }

    glm::quat AnimationSampler::ReinterpreteAsQuat(glm::vec3 vec)
    {
        return glm::normalize(glm::quat(0.f, vec.x, vec.y, vec.z));
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

            switch(channel.TargetPath)
            {
                case EAnimationTargetPath::Translation:
                    transform->SetTranslation(sampler.SampleVec(mPlaybackConfig.Cursor));
                    break;
                case EAnimationTargetPath::Rotation:
                    transform->SetRotation(sampler.SampleQuat(mPlaybackConfig.Cursor));
                    break;
                case EAnimationTargetPath::Scale:
                    transform->SetScale(sampler.SampleVec(mPlaybackConfig.Cursor));
                    break;
                default:
                    continue;
            }
            transform->RecalculateGlobalMatrix();
        }
    }
}  // namespace hsk