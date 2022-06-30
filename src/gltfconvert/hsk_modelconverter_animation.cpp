#include "../scenegraph/globalcomponents/hsk_animationdirector.hpp"
#include "hsk_modelconverter.hpp"
#include <map>

namespace hsk {
    void ModelConverter::LoadAnimations()
    {
        AnimationDirector* animDirector = mScene->GetComponent<AnimationDirector>();

        std::map<std::string_view, EAnimationInterpolation> interpolationMap = {
            {"LINEAR", EAnimationInterpolation::Linear}, {"STEP", EAnimationInterpolation::Step}, {"CUBICSPLINE", EAnimationInterpolation::Cubicspline}};


        std::map<std::string_view, EAnimationTargetPath> targetMap = {
            {"translation", EAnimationTargetPath::Translation}, {"rotation", EAnimationTargetPath::Rotation}, {"scale", EAnimationTargetPath::Scale}};

        auto interpolationNoMatch = interpolationMap.end();
        auto targetNoMatch        = targetMap.end();

        if(mGltfModel.animations.size() && !animDirector)
        {
            animDirector = mScene->MakeComponent<AnimationDirector>();
        }
        for(int32_t i = 0; i < mGltfModel.animations.size(); i++)
        {
            Animation animation;
            auto&     gltfAnimation = mGltfModel.animations[i];
            animation.SetName(gltfAnimation.name);

            animation.GetSamplers().reserve(gltfAnimation.samplers.size());
            animation.GetChannels().reserve(gltfAnimation.channels.size());

            std::map<int32_t, int32_t> samplerIndexMap;

            for(uint32_t samplerIndex = 0; i < gltfAnimation.samplers.size(); i++)
            {
                AnimationSampler sampler;
                auto&            gltfSampler = gltfAnimation.samplers[samplerIndex];

                auto interpolation = interpolationMap.find(gltfSampler.interpolation);
                if(interpolation != interpolationNoMatch)
                {
                    sampler.Interpolation = interpolation->second;
                }
                else
                {
                    sampler.Interpolation = EAnimationInterpolation::Linear;
                    logger()->warn("Model Load: In animation \"{}\" (#{}), sampler #{}: Unable to match \"{}\" to an interpolation type! Falling back to linear!",
                                   animation.GetName(), i, samplerIndex, gltfSampler.interpolation);
                }

                samplerIndexMap[samplerIndex] = animation.GetSamplers().size();
                animation.GetSamplers().push_back(std::move(sampler));
            }

            for(uint32_t channelIndex = 0; i < gltfAnimation.channels.size(); i++)
            {
                AnimationChannel channel;
                auto&            gltfChannel = gltfAnimation.channels[channelIndex];

                auto targetpath = targetMap.find(gltfChannel.target_path);
                if(targetpath != targetNoMatch)
                {
                    channel.TargetPath = targetpath->second;
                }
                else
                {
                    logger()->warn("Model Load: In animation \"{}\" (#{}), channel #{}: Unable to match \"{}\" to a target path! Skipping Channel!", animation.GetName(), i,
                                   channelIndex, gltfChannel.target_path);
                    continue;
                }
            }
        }
    }

}  // namespace hsk