#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>
#include <tinygltf/tiny_gltf.h>

namespace hsk {
    struct AnimationChannel
    {
        enum PathType
        {
            TRANSLATION,
            ROTATION,
            SCALE
        };
        PathType path         = PathType::TRANSLATION;
        Node*    node         = nullptr;
        uint32_t samplerIndex = -1;
    };

    struct AnimationSampler
    {
        enum InterpolationType
        {
            LINEAR,
            STEP,
            CUBICSPLINE
        };
        InterpolationType      interpolation = InterpolationType::LINEAR;
        std::vector<float>     inputs        = {};
        std::vector<glm::vec4> outputsVec4   = {};
    };

    class Animation : public SceneComponent, public NoMoveDefaults
    {
      public:
        std::string                   name     = {};
        std::vector<AnimationSampler> samplers = {};
        std::vector<AnimationChannel> channels = {};
        float                         start    = std::numeric_limits<float>::max();
        float                         end      = std::numeric_limits<float>::min();

        inline explicit Animation(Scene* scene) : SceneComponent(scene) {}

        void InitFromTinyGltfAnimation(const tinygltf::Model& model, const tinygltf::Animation& animation, int32_t index);

        bool Update(float time);
    };

}  // namespace hsk