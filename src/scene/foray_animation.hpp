#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../foray_basics.hpp"
#include "../foray_glm.hpp"
#include "foray_scene_declares.hpp"
#include <vector>

namespace foray::scene {
    /// @brief Interpolation mode defines how values are interpolated between keyframes
    enum class EAnimationInterpolation
    {
        Linear,
        Step,
        Cubicspline
    };

    /// @brief Target path defines which aspect of a nodes transforms the animation channel targets
    enum class EAnimationTargetPath
    {
        Translation,
        Rotation,
        Scale
    };

    /// @brief A set of values at a time point
    struct AnimationKeyframe
    {
      public:
        inline AnimationKeyframe() {}
        inline AnimationKeyframe(float time, const glm::vec4& value) : Time(time), Value(value) {}
        inline AnimationKeyframe(float time, const glm::vec4& value, const glm::vec4& intangent, const glm::vec4& outtangent)
            : Time(time), Value(value), InTangent(intangent), OutTangent(outtangent)
        {
        }

        float     Time = 0.f;
        glm::vec4 Value;
        glm::vec4 InTangent;
        glm::vec4 OutTangent;
    };

    /// @brief A collection of keyframes
    struct AnimationSampler
    {
      public:
        /// @brief Get an interpolated sample
        /// @param time Timepoint
        /// @return Translation or Scale vector
        glm::vec3 SampleVec(float time) const;
        /// @brief Get an interpolated sample
        /// @param time Timepoint
        /// @return Rotation quaternion
        glm::quat SampleQuat(float time) const;

        static glm::vec4 InterpolateStep(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper);
        static glm::vec4 InterpolateLinear(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper);
        static glm::quat InterpolateLinearQuat(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper);
        static glm::vec4 InterpolateCubicSpline(float time, const AnimationKeyframe* lower, const AnimationKeyframe* upper);
        static glm::quat ReinterpreteAsQuat(glm::vec4);

        EAnimationInterpolation        Interpolation = {};
        std::vector<AnimationKeyframe> Keyframes     = {};

      protected:
        void SelectKeyframe(float time, const AnimationKeyframe*& lower, const AnimationKeyframe*& upper) const;
    };

    /// @brief A channel is the animation of a single node property
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

    /// @brief Represents an animation, defined by atleast one channel affecting one node property each
    class Animation
    {
      public:
        FORAY_PROPERTY_R(Name)
        FORAY_PROPERTY_R(Samplers)
        FORAY_PROPERTY_R(Channels)
        FORAY_PROPERTY_V(Start)
        FORAY_PROPERTY_V(End)
        FORAY_PROPERTY_V(PlaybackConfig)

        /// @brief Applies current playback state
        void Update(const base::FrameRenderInfo&);

      protected:
        /// @brief Animation name
        std::string mName;
        /// @brief Collection of samplers. These contain raw keyframe values
        std::vector<AnimationSampler> mSamplers;
        /// @brief Collection of channels. These bind keyframe values from samplers to node properties
        std::vector<AnimationChannel> mChannels;
        /// @brief The lowest keyframe time value
        float mStart = {};
        /// @brief The highest keyframe time value
        float mEnd = {};
        /// @brief Configuration representing current playback state
        PlaybackConfig mPlaybackConfig;
    };

}  // namespace foray::scene