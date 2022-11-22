#include "foray_animation.hpp"
#include "components/foray_transform.hpp"
#include "foray_node.hpp"

namespace foray::scene {

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

        const AnimationKeyframe* lower = nullptr;
        const AnimationKeyframe* upper = nullptr;
        SelectKeyframe(time, lower, upper);

        if(lower == upper)  // happens at start or end of animation. Fallback to step interpolation in this case
        {
            return InterpolateStep(time, lower, upper);
        }

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

        const AnimationKeyframe* lower = {};
        const AnimationKeyframe* upper = {};
        SelectKeyframe(time, lower, upper);

        if(lower == upper)
        {
            return ReinterpreteAsQuat(InterpolateStep(time, lower, upper));
        }

        switch(Interpolation)
        {
            case EAnimationInterpolation::Step: {
                return ReinterpreteAsQuat(InterpolateStep(time, lower, upper));
            }
            case EAnimationInterpolation::Linear: {
                return InterpolateLinearQuat(time, lower, upper);
            }
            case EAnimationInterpolation::Cubicspline: {
                return ReinterpreteAsQuat(InterpolateCubicSpline(time, lower, upper));
            }
        }

        return glm::quat();
    }

    void AnimationSampler::SelectKeyframe(float time, const AnimationKeyframe*& lower, const AnimationKeyframe*& upper) const
    {
        int32_t lowerIndex = 0;
        int32_t upperIndex = Keyframes.size() - 1;

        while(true)
        {
            int32_t nextIndex = lowerIndex + 1;
            if(nextIndex >= Keyframes.size() || Keyframes[nextIndex].Time > time)
            {
                break;
            }
            lowerIndex++;
        }
        while(true)
        {
            int32_t nextIndex = upperIndex - 1;
            if(nextIndex < 0 || Keyframes[nextIndex].Time < time)
            {
                break;
            }
            upperIndex--;
        }

        lower = &Keyframes[lowerIndex];
        upper = &Keyframes[upperIndex];
    }

    glm::vec4 AnimationSampler::InterpolateStep(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper)
    {
        return lower->Value;
    }

    glm::vec4 AnimationSampler::InterpolateLinear(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper)
    {
        float dist = upper->Time - lower->Time;
        float t    = (time - lower->Time) / dist;
        return glm::mix(upper->Value, lower->Value, t);
    }

    glm::quat AnimationSampler::InterpolateLinearQuat(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper)
    {
        glm::quat lowerQuat = ReinterpreteAsQuat(lower->Value);
        glm::quat upperQuat = ReinterpreteAsQuat(upper->Value);
        float     dist      = upper->Time - lower->Time;
        float     t         = (time - lower->Time) / dist;
        return glm::normalize(glm::slerp(upperQuat, lowerQuat, t));
    }

    glm::vec4 AnimationSampler::InterpolateCubicSpline(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper)
    {
        float dist     = upper->Time - lower->Time;
        float t        = (time - lower->Time) / dist;
        float tSquared = t * t;
        float tCubed   = t * tSquared;
        return (2 * tCubed - 3 * tSquared + 1) * lower->Value + dist * (tCubed - 2 * tSquared + t) * lower->OutTangent + (-2 * tCubed + 3 * tSquared) * upper->Value
               + dist * (tCubed - tSquared) * upper->InTangent;
    }

    glm::quat AnimationSampler::ReinterpreteAsQuat(glm::vec4 vec)
    {
        return glm::normalize(glm::quat(vec.w, vec.x, vec.y, vec.z));
    }

    void Animation::Update(const base::FrameRenderInfo& updateInfo)
    {
        if(mPlaybackConfig.Enable)
        {

            float delta = mPlaybackConfig.PlaybackSpeed;
            if(mPlaybackConfig.ConstantDelta != 0.f)
            {
                delta *= mPlaybackConfig.ConstantDelta;
            }
            else
            {
                delta *= updateInfo.GetFrameTime();
            }
            float newCursor = mPlaybackConfig.Cursor + delta;
            if(mPlaybackConfig.Loop)
            {
                float duration = mEnd - mStart;
                while(newCursor < mStart)
                {
                    newCursor += duration;
                }
                while(newCursor > mEnd)
                {
                    newCursor -= duration;
                }
            }
            mPlaybackConfig.Cursor = newCursor;
        }
        for(auto& channel : mChannels)
        {
            auto  transform = channel.Target->GetTransform();
            auto& sampler   = mSamplers[channel.SamplerIndex];

#ifdef WIN32
#define isnanf _isnanf
#endif

            switch(channel.TargetPath)
            {
                case EAnimationTargetPath::Translation: {
                    glm::vec3 translation = sampler.SampleVec(mPlaybackConfig.Cursor);
                    if(isnanf(translation.x) || isnanf(translation.y) || isnanf(translation.z))
                    {
                        FORAY_THROWFMT("NAN: ({}|{}|{}) \"{}\"", translation.x, translation.y, translation.z, "Translation");
                    }
                    transform->SetTranslation(translation);
                    break;
                }
                case EAnimationTargetPath::Rotation: {
                    glm::qua quat = sampler.SampleQuat(mPlaybackConfig.Cursor);
                    if(isnanf(quat.x) || isnanf(quat.y) || isnanf(quat.z) || isnanf(quat.w))
                    {
                        FORAY_THROWFMT("NAN: ({}|{}|{}|{}) \"{}\"", quat.x, quat.y, quat.z, quat.w, "Rotation");
                    }
                    transform->SetRotation(quat);
                    break;
                }
                case EAnimationTargetPath::Scale: {
                    glm::vec3 scale = sampler.SampleVec(mPlaybackConfig.Cursor);
                    if(isnanf(scale.x) || isnanf(scale.y) || isnanf(scale.z))
                    {
                        FORAY_THROWFMT("NAN: ({}|{}|{}) \"{}\"", scale.x, scale.y, scale.z, "Scale");
                    }
                    transform->SetScale(scale);
                    break;
                }
                default:
                    continue;
            }
            transform->RecalculateGlobalMatrix();
        }
    }
}  // namespace foray::scene