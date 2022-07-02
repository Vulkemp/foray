#include "../scenegraph/globalcomponents/hsk_animationdirector.hpp"
#include "hsk_modelconverter.hpp"
#include <map>
#include <spdlog/fmt/fmt.h>

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
            if(!animation.GetName().length())
            {
                animation.SetName(fmt::format("Animation #{}", i));
            }

            animation.GetSamplers().reserve(gltfAnimation.samplers.size());
            animation.GetChannels().reserve(gltfAnimation.channels.size());

            std::map<int32_t, int32_t> samplerIndexMap;

            auto indexMapNoMatch = samplerIndexMap.end();

            for(int32_t samplerIndex = 0; samplerIndex < gltfAnimation.samplers.size(); samplerIndex++)
            {
                TranslateAnimationSampler(animation, gltfAnimation, samplerIndex, interpolationMap, samplerIndexMap);
            }

            for(int32_t channelIndex = 0; channelIndex < gltfAnimation.channels.size(); channelIndex++)
            {
                AnimationChannel channel;
                auto&            gltfChannel = gltfAnimation.channels[channelIndex];

                auto samplerIndex = samplerIndexMap.find(gltfChannel.sampler);

                if(samplerIndex != indexMapNoMatch)
                {
                    channel.SamplerIndex = samplerIndex->second;
                }
                else
                {
                    logger()->warn("Model Load: In animation \"{}\", channel #{}: Referencing invalid animation sampler #{}! Skipping Channel!", animation.GetName(), channelIndex,
                                   gltfChannel.sampler);
                    continue;
                }

                if(gltfChannel.target_node >= 0 || gltfChannel.target_node < mIndexBindings.Nodes.size())
                {
                    channel.Target = mIndexBindings.Nodes[gltfChannel.target_node];
                }
                else
                {
                    logger()->warn("Model Load: In animation \"{}\", channel #{}: Target Node index #{} out of bounds! Skipping Channel!", animation.GetName(), channelIndex,
                                   gltfChannel.target_node);
                    continue;
                }


                auto targetpath = targetMap.find(gltfChannel.target_path);
                if(targetpath != targetNoMatch)
                {
                    channel.TargetPath = targetpath->second;
                }
                else
                {
                    logger()->warn("Model Load: In animation \"{}\", channel #{}: Unable to match \"{}\" to a target path! Skipping Channel!", animation.GetName(), channelIndex,
                                   gltfChannel.target_path);
                    continue;
                }

                animation.GetChannels().push_back(std::move(channel));
            }

            if(!animation.GetChannels().size() || !animation.GetSamplers().size())
            {
                logger()->warn("Model Load: Animation \"{}\" without samplers or channels, skipping!", animation.GetName());
                continue;
            }
            animDirector->GetAnimations().push_back(animation);
        }
    }

    void ModelConverter::TranslateAnimationSampler(Animation&                                                 animation,
                                                   const tinygltf::Animation&                                 gltfAnimation,
                                                   int32_t                                                    samplerIndex,
                                                   const std::map<std::string_view, EAnimationInterpolation>& interpolationMap,
                                                   std::map<int, int>&                                        samplerIndexMap)
    {
        AnimationSampler sampler;
        auto&            gltfSampler = gltfAnimation.samplers[samplerIndex];

        // Read sampler input time values
        {
            if(gltfSampler.input < 0)
            {
                logger()->warn("Model Load: In animation \"{}\", sampler #{}: No input accessor provided! Skipping sampler!", animation.GetName(), samplerIndex);
                return;
            }
            const tinygltf::Accessor&   accessor   = mGltfModel.accessors[gltfSampler.input];
            const tinygltf::BufferView& bufferView = mGltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer&     buffer     = mGltfModel.buffers[bufferView.buffer];

            if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                logger()->warn("Model Load: In animation \"{}\", sampler #{}: Input Accessor of unrecognised type {}! Skipping sampler!", animation.GetName(), samplerIndex,
                               accessor.componentType);
                return;
            }

            const float* buf = reinterpret_cast<const float*>(buffer.data.data() + (accessor.byteOffset + bufferView.byteOffset));
            for(size_t index = 0; index < accessor.count; index++)
            {
                sampler.Keyframes.push_back(AnimationKeyframe{.Time=buf[index],.Value={}});
            }

            for(auto keyframe : sampler.Keyframes)
            {
                animation.SetStart(std::min(animation.GetStart(), keyframe.Time));
                animation.SetEnd(std::max(animation.GetEnd(), keyframe.Time));
            }
        }


        auto interpolation = interpolationMap.find(gltfSampler.interpolation);
        if(interpolation != interpolationMap.end())
        {
            sampler.Interpolation = interpolation->second;
        }
        else
        {
            sampler.Interpolation = EAnimationInterpolation::Linear;
            logger()->warn("Model Load: In animation \"{}\", sampler #{}: Unable to match \"{}\" to an interpolation type! Falling back to linear!", animation.GetName(),
                           samplerIndex, gltfSampler.interpolation);
        }

        // Read sampler output T/R/S values
        {
            if(gltfSampler.input < 0)
            {
                logger()->warn("Model Load: In animation \"{}\", sampler #{}: No input accessor provided! Skipping sampler!", animation.GetName(), samplerIndex);
                return;
            }

            const tinygltf::Accessor&   accessor   = mGltfModel.accessors[gltfSampler.output];
            const tinygltf::BufferView& bufferView = mGltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer&     buffer     = mGltfModel.buffers[bufferView.buffer];

            if (accessor.count != sampler.Keyframes.size()){
                logger()->warn("Model Load: In animation \"{}\", sampler #{}: Output Accessor count does not match input count! Skipping sampler!", animation.GetName(), samplerIndex,
                               accessor.componentType);
                return;
            }
            if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                logger()->warn("Model Load: In animation \"{}\", sampler #{}: Output Accessor of unrecognised type {}! Skipping sampler!", animation.GetName(), samplerIndex,
                               accessor.componentType);
                return;
            }

            switch(accessor.type)
            {
                case TINYGLTF_TYPE_VEC3: {
                    const glm::vec3* buf = reinterpret_cast<const glm::vec3*>(buffer.data.data() + (accessor.byteOffset + bufferView.byteOffset));
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        glm::vec3 value = buf[index];
                        sampler.Keyframes[index].Value = glm::vec4(value, 1.f);
                    }
                    break;
                }
                case TINYGLTF_TYPE_VEC4: {
                    const glm::vec4* buf = reinterpret_cast<const glm::vec4*>(buffer.data.data() + (accessor.byteOffset + bufferView.byteOffset));
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        glm::vec4 value = buf[index];
                        sampler.Keyframes[index].Value = value;
                    }
                    break;
                }
                default: {
                    logger()->warn("Model Load: In animation \"{}\", sampler #{}: Output Accessor of unrecognised type {}! Skipping sampler!", animation.GetName(), samplerIndex,
                                   accessor.type);
                    return;
                }
            }
        }

        samplerIndexMap[samplerIndex] = animation.GetSamplers().size();
        animation.GetSamplers().push_back(std::move(sampler));
    }

}  // namespace hsk