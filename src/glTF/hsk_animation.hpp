#pragma once
#include "hsk_glTF_declares.hpp"
#include <glm/glm.hpp>

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

    struct Animation
    {
        std::string                   name     = {};
        std::vector<AnimationSampler> samplers = {};
        std::vector<AnimationChannel> channels = {};
        float                         start    = std::numeric_limits<float>::max();
        float                         end      = std::numeric_limits<float>::min();
    };

}  // namespace hsk