#include "hsk_animation.hpp"
#include "hsk_scene.hpp"
#include "spdlog/fmt/fmt.h"

namespace hsk {

    void Animation::InitFromTinyGltfAnimation(const tinygltf::Model& model, const tinygltf::Animation& anim, int32_t index)
    {
        name = anim.name;
        if(name.empty())
        {
            name = fmt::format("Animation #{}", index);
        }

        // Samplers
        for(auto& samp : anim.samplers)
        {
            AnimationSampler sampler{};

            if(samp.interpolation == "LINEAR")
            {
                sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
            }
            if(samp.interpolation == "STEP")
            {
                sampler.interpolation = AnimationSampler::InterpolationType::STEP;
            }
            if(samp.interpolation == "CUBICSPLINE")
            {
                sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
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
                    sampler.inputs.push_back(buf[index]);
                }

                for(auto input : sampler.inputs)
                {
                    if(input < start)
                    {
                        start = input;
                    };
                    if(input > end)
                    {
                        end = input;
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
                            sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            sampler.outputsVec4.push_back(buf[index]);
                        }
                        break;
                    }
                    default: {
                        throw Exception("Unknown accessor type: {}", accessor.type);
                    }
                }
            }

            samplers.push_back(sampler);
        }

        // Channels
        for(auto& source : anim.channels)
        {
            AnimationChannel channel{};

            if(source.target_path == "rotation")
            {
                channel.path = AnimationChannel::PathType::ROTATION;
            }
            if(source.target_path == "translation")
            {
                channel.path = AnimationChannel::PathType::TRANSLATION;
            }
            if(source.target_path == "scale")
            {
                channel.path = AnimationChannel::PathType::SCALE;
            }
            if(source.target_path == "weights")
            {
                throw Exception("Weights are not supported");
            }
            channel.samplerIndex = source.sampler;
            channel.node         = mOwningScene->GetNodeByIndex(source.target_node);
            if(!channel.node)
            {
                continue;
            }

            channels.push_back(channel);
        }
    }

    bool Animation::Update(float time)
    {
        bool updated = false;
        for(auto& channel : channels)
        {
            AnimationSampler& sampler = samplers[channel.samplerIndex];
            if(sampler.inputs.size() > sampler.outputsVec4.size())
            {
                continue;
            }

            for(size_t i = 0; i < sampler.inputs.size() - 1; i++)
            {
                if((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1]))
                {
                    float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                    if(u <= 1.0f)
                    {
                        switch(channel.path)
                        {
                            case AnimationChannel::PathType::TRANSLATION: {
                                glm::vec4 trans                      = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                                channel.node->mTransform.Translation = glm::vec3(trans);
                                break;
                            }
                            case AnimationChannel::PathType::SCALE: {
                                glm::vec4 trans                = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                                channel.node->mTransform.Scale = glm::vec3(trans);
                                break;
                            }
                            case AnimationChannel::PathType::ROTATION: {
                                glm::quat q1;
                                q1.x = sampler.outputsVec4[i].x;
                                q1.y = sampler.outputsVec4[i].y;
                                q1.z = sampler.outputsVec4[i].z;
                                q1.w = sampler.outputsVec4[i].w;
                                glm::quat q2;
                                q2.x                              = sampler.outputsVec4[i + 1].x;
                                q2.y                              = sampler.outputsVec4[i + 1].y;
                                q2.z                              = sampler.outputsVec4[i + 1].z;
                                q2.w                              = sampler.outputsVec4[i + 1].w;
                                channel.node->mTransform.Rotation = glm::normalize(glm::slerp(q1, q2, u));
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