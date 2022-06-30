#pragma once
#include "../../hsk_glm.hpp"
#include "../hsk_component.hpp"

namespace hsk {

    enum EAnimationInterpolation
    {
        Linear,
        Step,
        Cubicspline
    };

    enum EAnimationTargetPath
    {
        Translation,
        Rotation,
        Scale
    };

    struct AnimationSampler
    {
      public:
        std::vector<float>      Inputs        = {};
        EAnimationInterpolation Interpolation = {};
        std::vector<glm::vec4>  Outputs       = {};
    };
    struct AnimationChannel
    {
      public:
        int32_t              SamplerIndex = {-1};
        Node*                Target       = {nullptr};
        EAnimationTargetPath TargetPath   = {};
    };

    struct PlaybackConfig
    {
      public:
        /// @brief If false, playback is paused
        bool Enable = false;
        /// @brief Speed multiplier
        float PlaybackSpeed = 1.f;
        /// @brief Current playback position used for interpolation
        float Cursor = 0.f;
    };

    class Animation
    {
      public:
        HSK_PROPERTY_ALL(Name)
        HSK_PROPERTY_ALLGET(Samplers)
        HSK_PROPERTY_ALLGET(Channels)
        HSK_PROPERTY_ALL(Start)
        HSK_PROPERTY_ALL(End)
        HSK_PROPERTY_ALL(PlaybackConfig)

      protected:
        std::string                   mName;
        std::vector<AnimationSampler> mSamplers;
        std::vector<AnimationChannel> mChannels;
        float                         mStart = {};
        float                         mEnd   = {};
        PlaybackConfig                mPlaybackConfig;
    };

    class AnimationDirector : public GlobalComponent
    {
      public:
        HSK_PROPERTY_ALLGET(Animations)
        HSK_PROPERTY_ALL(PlaybackConfig)
      protected:
        std::vector<Animation> mAnimations;
        PlaybackConfig         mPlaybackConfig;
    };
}  // namespace hsk