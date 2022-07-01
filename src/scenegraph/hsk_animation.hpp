#pragma once
#include "../hsk_basics.hpp"
#include "../hsk_glm.hpp"
#include "hsk_scenegraph_declares.hpp"

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

    struct AnimationKeyframe
    {
      public:
        float     Time;
        glm::vec4 Value;
    };

    struct AnimationSampler
    {
      public:
        glm::vec4 SampleVec4(float time) const;
        glm::vec4 SampleQuat(float time) const;

        EAnimationInterpolation        Interpolation = {};
        std::vector<AnimationKeyframe> Keyframes     = {};
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
        bool Enable = true;
        /// @brief The animation is looped if set to true
        bool Loop = true;
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

        void Update(const FrameUpdateInfo&);

      protected:
        std::string                   mName;
        std::vector<AnimationSampler> mSamplers;
        std::vector<AnimationChannel> mChannels;
        float                         mStart = {};
        float                         mEnd   = {};
        PlaybackConfig                mPlaybackConfig;
    };

}  // namespace hsk