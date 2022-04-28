#include "hsk_animation.hpp"
#include "hsk_scene.hpp"
#include "spdlog/fmt/fmt.h"

namespace hsk {

    void Animation::InitFromTinyGltfAnimation(const tinygltf::Model& model, const tinygltf::Animation& anim, int32_t index)
    {
        mName = anim.name;
        if(mName.empty())
        {
            mName = fmt::format("Animation #{}", index);
        }

        // Samplers
        for(auto& samp : anim.samplers)
        {
            AnimationSampler sampler{};

            if(samp.interpolation == "LINEAR")
            {
                sampler.Interpolation = AnimationSampler::EInterpolationType::LINEAR;
            }
            if(samp.interpolation == "STEP")
            {
                sampler.Interpolation = AnimationSampler::EInterpolationType::STEP;
            }
            if(samp.interpolation == "CUBICSPLINE")
            {
                sampler.Interpolation = AnimationSampler::EInterpolationType::CUBICSPLINE;
            }

            // Read sampler input time values
            {
                const tinygltf::Accessor&   accessor   = model.accessors[samp.input];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = model.buffers[bufferView.buffer];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                const void*  dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                const float* buf     = static_cast<const float*>(dataPtr);
                for(size_t index = 0; index < accessor.count; index++)
                {
                    sampler.Inputs.push_back(buf[index]);
                }

                for(auto input : sampler.Inputs)
                {
                    if(input < mStart)
                    {
                        mStart = input;
                    };
                    if(input > mEnd)
                    {
                        mEnd = input;
                    }
                }
            }

            // Read sampler output T/R/S values
            {
                const tinygltf::Accessor&   accessor   = model.accessors[samp.output];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = model.buffers[bufferView.buffer];

                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                switch(accessor.type)
                {
                    case TINYGLTF_TYPE_VEC3: {
                        const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            sampler.Outputs.push_back(glm::vec4(buf[index], 0.0f));
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            sampler.Outputs.push_back(buf[index]);
                        }
                        break;
                    }
                    default: {
                        throw Exception("Unknown accessor type: {}", accessor.type);
                    }
                }
            }

            mSamplers.push_back(sampler);
        }

        // Channels
        for(auto& source : anim.channels)
        {
            AnimationChannel channel{};

            if(source.target_path == "rotation")
            {
                channel.Path = AnimationChannel::EPathType::ROTATION;
            }
            if(source.target_path == "translation")
            {
                channel.Path = AnimationChannel::EPathType::TRANSLATION;
            }
            if(source.target_path == "scale")
            {
                channel.Path = AnimationChannel::EPathType::SCALE;
            }
            if(source.target_path == "weights")
            {
                throw Exception("Weights are not supported");
            }
            channel.SamplerIndex = source.sampler;
            channel.Target         = Owner()->GetNodeByIndex(source.target_node);
            if(!channel.Target)
            {
                continue;
            }

            mChannels.push_back(channel);
        }
    }

    bool Animation::Update(float time)
    {
        bool updated = false;
        for(auto& channel : mChannels)
        {
            AnimationSampler& sampler = mSamplers[channel.SamplerIndex];
            if(sampler.Inputs.size() > sampler.Outputs.size())
            {
                continue;
            }

            for(size_t i = 0; i < sampler.Inputs.size() - 1; i++)
            {
                if((time >= sampler.Inputs[i]) && (time <= sampler.Inputs[i + 1]))
                {
                    float u = std::max(0.0f, time - sampler.Inputs[i]) / (sampler.Inputs[i + 1] - sampler.Inputs[i]);
                    if(u <= 1.0f)
                    {
                        switch(channel.Path)
                        {
                            case AnimationChannel::EPathType::TRANSLATION: {
                                glm::vec4 trans                      = glm::mix(sampler.Outputs[i], sampler.Outputs[i + 1], u);
                                channel.Target->GetTransform().Translation = glm::vec3(trans);
                                break;
                            }
                            case AnimationChannel::EPathType::SCALE: {
                                glm::vec4 trans                = glm::mix(sampler.Outputs[i], sampler.Outputs[i + 1], u);
                                channel.Target->GetTransform().Scale = glm::vec3(trans);
                                break;
                            }
                            case AnimationChannel::EPathType::ROTATION: {
                                glm::quat q1;
                                q1.x = sampler.Outputs[i].x;
                                q1.y = sampler.Outputs[i].y;
                                q1.z = sampler.Outputs[i].z;
                                q1.w = sampler.Outputs[i].w;
                                glm::quat q2;
                                q2.x                              = sampler.Outputs[i + 1].x;
                                q2.y                              = sampler.Outputs[i + 1].y;
                                q2.z                              = sampler.Outputs[i + 1].z;
                                q2.w                              = sampler.Outputs[i + 1].w;
                                channel.Target->GetTransform().Rotation = glm::normalize(glm::slerp(q1, q2, u));
                                break;
                            }
                        }
                        updated = true;
                    }
                }
            }
        }
        return updated;
    }
}  // namespace hsk