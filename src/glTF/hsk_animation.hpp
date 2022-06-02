#pragma once
#include "hsk_gltf_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <glm/glm.hpp>
#include <tinygltf/tiny_gltf.h>

namespace hsk {
    struct AnimationChannel
    {
        enum EPathType
        {
            TRANSLATION,
            ROTATION,
            SCALE
        };
        EPathType Path         = EPathType::TRANSLATION;
        Node*     Target       = nullptr;
        int32_t  SamplerIndex = -1;
    };

    struct AnimationSampler
    {
        enum EInterpolationType
        {
            LINEAR,
            STEP,
            CUBICSPLINE
        };
        EInterpolationType     Interpolation = EInterpolationType::LINEAR;
        std::vector<float>     Inputs        = {};
        std::vector<glm::vec4> Outputs       = {};
    };

    class Animation : public SceneComponent, public NoMoveDefaults
    {
      public:
        inline explicit Animation(Scene* scene) : SceneComponent(scene) {}

        void InitFromTinyGltfAnimation(const tinygltf::Model& model, const tinygltf::Animation& animation, int32_t index);

        bool Update(float time);

        HSK_PROPERTY_ALL(Name)
        HSK_PROPERTY_ALL(Samplers)
        HSK_PROPERTY_ALL(Channels)
        HSK_PROPERTY_ALL(Start)
        HSK_PROPERTY_ALL(End)

      protected:
        std::string                   mName     = {};
        std::vector<AnimationSampler> mSamplers = {};
        std::vector<AnimationChannel> mChannels = {};
        float                         mStart    = std::numeric_limits<float>::max();
        float                         mEnd      = std::numeric_limits<float>::min();
    };

}  // namespace hsk