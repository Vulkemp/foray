#include "hsk_scene.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../hsk_vmaHelper.hpp"
#include "hsk_mesh.hpp"
#include "hsk_node.hpp"
#include "hsk_skin.hpp"
#include "hsk_texture.hpp"
#include "hsk_animation.hpp"

namespace hsk {
    void Scene::destroy()
    {
        if(vertices.buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(mContext.Allocator, vertices.buffer, vertices.allocation);
            vertices.buffer = VK_NULL_HANDLE;
        }
        if(indices.buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(mContext.Allocator, indices.buffer, indices.allocation);
            indices.buffer = VK_NULL_HANDLE;
        }

        // Texture has a destructor which cleans up unmanaged buffers automatically, so this is safe
        mTextures.resize(0);

        // Texturesamplers are dumb structs, so this is safe
        textureSamplers.resize(0);

        // TODO: Properly kill nodes

        // Materials are dumb structs, so this is safe
        mMaterials.resize(0);

        // TODO: Kill animations

        extensions.resize(0);

        // TODO: Kill skins
    }

    void Scene::loadFromFile(std::string filename, float scale)
    {
        tinygltf::Model    gltfModel;
        tinygltf::TinyGLTF gltfContext;
        std::string        error;
        std::string        warning;

        bool   binary = false;
        size_t extpos = filename.rfind('.', filename.length());
        if(extpos != std::string::npos)
        {
            binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
        }

        bool fileLoaded =
            binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str()) : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

        std::vector<uint32_t> indexBuffer;
        std::vector<Vertex>   vertexBuffer;

        if(fileLoaded)
        {
            loadTextureSamplers(gltfModel);
            loadTextures(gltfModel);
            loadMaterials(gltfModel);
            // TODO: scene handling with no default scene
            const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
            for(size_t i = 0; i < scene.nodes.size(); i++)
            {
                const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
                loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
            }
            // TODO: Load Animations
            // if(gltfModel.animations.size() > 0)
            // {
            //     loadAnimations(gltfModel);
            // }
            loadSkins(gltfModel);

            for(auto& node : linearNodes)
            {
                // Assign skins
                if(node->skinIndex > -1)
                {
                    node->skin = skins[node->skinIndex].get();
                }
                // Initial pose
                if(node->mesh)
                {
                    node->update();
                }
            }
        }
        else
        {
            throw Exception("Could not load gltf file: {}", error);
        }

        extensions = gltfModel.extensionsUsed;

        size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
        size_t indexBufferSize  = indexBuffer.size() * sizeof(uint32_t);
        indices.count           = static_cast<uint32_t>(indexBuffer.size());

        assert(vertexBufferSize > 0);

        struct StagingBuffer
        {
            VkBuffer      buffer;
            VmaAllocation allocation;
        } vertexStaging, indexStaging;

        VmaAllocationCreateInfo allocInfo;
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        // Create staging buffers
        // Vertex data

        createBuffer(mContext.Allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, allocInfo, &vertexStaging.allocation, vertexBufferSize, &vertexStaging.buffer, vertexBuffer.data());
        // Index data
        if(indexBufferSize > 0)
        {
            createBuffer(mContext.Allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, allocInfo, &indexStaging.allocation, indexBufferSize, &indexStaging.buffer, indexBuffer.data());
        }

        // Create device local buffers
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        // Vertex buffer
        createBuffer(mContext.Allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo, &vertices.allocation, vertexBufferSize, &vertices.buffer);
        // Index buffer
        if(indexBufferSize > 0)
        {
            createBuffer(mContext.Allocator, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo, &indices.allocation, indexBufferSize, &indices.buffer);
        }

        // Copy from staging buffers
        VkCommandBuffer copyCmd = createCommandBuffer(mContext.Device, mContext.TransferCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copyRegion = {};

        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

        if(indexBufferSize > 0)
        {
            copyRegion.size = indexBufferSize;
            vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
        }

        flushCommandBuffer(mContext.Device, mContext.TransferCommandPool, copyCmd, mContext.TransferQueue, true);

        vmaDestroyBuffer(mContext.Allocator, vertexStaging.buffer, vertexStaging.allocation);
        if(indexBufferSize > 0)
        {
            vmaDestroyBuffer(mContext.Allocator, indexStaging.buffer, indexStaging.allocation);
        }

        getSceneDimensions();
    }

    void Scene::loadTextureSamplers(tinygltf::Model& gltfModel)
    {
        for(tinygltf::Sampler smpl : gltfModel.samplers)
        {
            TextureSampler sampler{};
            sampler.MinFilter    = TextureSampler::getVkFilterMode(smpl.minFilter);
            sampler.MagFilter    = TextureSampler::getVkFilterMode(smpl.magFilter);
            sampler.AddressModeU = TextureSampler::getVkWrapMode(smpl.wrapS);
            sampler.AddressModeV = TextureSampler::getVkWrapMode(smpl.wrapT);
            sampler.AddressModeW = sampler.AddressModeV;
            textureSamplers.push_back(sampler);
        }
    }

    void Scene::loadTextures(tinygltf::Model& gltfModel)
    {
        for(tinygltf::Texture& tex : gltfModel.textures)
        {
            tinygltf::Image image = gltfModel.images[tex.source];
            TextureSampler  textureSampler;
            if(tex.sampler == -1)
            {
                // No sampler specified, use a default one
                textureSampler.MagFilter    = VK_FILTER_LINEAR;
                textureSampler.MinFilter    = VK_FILTER_LINEAR;
                textureSampler.AddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                textureSampler.AddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                textureSampler.AddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
            else
            {
                textureSampler = textureSamplers[tex.sampler];
            }
            std::unique_ptr<Texture> texture = std::make_unique<Texture>(this);
            texture->InitFromTinyGltfImage(image, textureSampler);
            mTextures.push_back(std::move(texture));
        }
    }

    void Scene::loadMaterials(tinygltf::Model& gltfModel)
    {
        for(auto& gltfmat : gltfModel.materials)
        {
            Material material(this);
            material.InitFromTinyGltfMaterial(gltfmat);
            mMaterials.push_back(material);
        }
    }

    void Scene::loadNode(Node*                  parent,
                         const tinygltf::Node&  node,
                         uint32_t               nodeIndex,
                         const tinygltf::Model& model,
                         std::vector<uint32_t>& indexBuffer,
                         std::vector<Vertex>&   vertexBuffer,
                         float                  globalscale)
    {
        std::unique_ptr<Node> newNode = std::make_unique<Node>();
        newNode->index                = nodeIndex;
        newNode->parent               = parent;
        newNode->name                 = node.name;
        newNode->skinIndex            = node.skin;
        newNode->matrix               = glm::mat4(1.0f);

        // Generate local node matrix
        glm::vec3 translation = glm::vec3(0.0f);
        if(node.translation.size() == 3)
        {
            translation          = glm::make_vec3(node.translation.data());
            newNode->translation = translation;
        }
        glm::mat4 rotation = glm::mat4(1.0f);
        if(node.rotation.size() == 4)
        {
            glm::quat q       = glm::make_quat(node.rotation.data());
            newNode->rotation = glm::mat4(q);
        }
        glm::vec3 scale = glm::vec3(1.0f);
        if(node.scale.size() == 3)
        {
            scale          = glm::make_vec3(node.scale.data());
            newNode->scale = scale;
        }
        if(node.matrix.size() == 16)
        {
            newNode->matrix = glm::make_mat4x4(node.matrix.data());
        };

        // Node with children
        if(node.children.size() > 0)
        {
            for(size_t i = 0; i < node.children.size(); i++)
            {
                loadNode(newNode.get(), model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
            }
        }

        // Node contains mesh data
        if(node.mesh > -1)
        {
            // TODO: Consider using instancing here!
            newNode->mesh = std::make_unique<Mesh>(this);
            newNode->mesh->InitFromTinyGltfMesh(model, model.meshes[node.mesh], indexBuffer, vertexBuffer);
        }
        if(parent)
        {
            parent->children.push_back(newNode.get());
        }
        else
        {
            nodes.push_back(newNode.get());
        }
        linearNodes.push_back(std::move(newNode));
    }

    void Scene::loadSkins(const tinygltf::Model& gltfModel)
    {
        for(const tinygltf::Skin& source : gltfModel.skins)
        {
            std::unique_ptr<Skin> newSkin = std::make_unique<Skin>();
            newSkin->name                 = source.name;

            // Find skeleton root node
            if(source.skeleton > -1)
            {
                newSkin->skeletonRoot = GetNodeByIndex(source.skeleton);
            }

            // Find joint nodes
            for(int jointIndex : source.joints)
            {
                Node* node = GetNodeByIndex(jointIndex);
                if(node)
                {
                    newSkin->joints.push_back(node);
                }
            }

            // Get inverse bind matrices from buffer
            if(source.inverseBindMatrices > -1)
            {
                const tinygltf::Accessor&   accessor   = gltfModel.accessors[source.inverseBindMatrices];
                const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = gltfModel.buffers[bufferView.buffer];
                newSkin->inverseBindMatrices.resize(accessor.count);
                memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
            }

            skins.push_back(std::move(newSkin));
        }
    }

    void Scene::loadAnimations(tinygltf::Model& gltfModel)
    {
        for(tinygltf::Animation& anim : gltfModel.animations)
        {
            Animation animation{};
            animation.name = anim.name;
            if(anim.name.empty())
            {
                animation.name = std::to_string(animations.size());
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
                    const tinygltf::Accessor&   accessor   = gltfModel.accessors[samp.input];
                    const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer&     buffer     = gltfModel.buffers[bufferView.buffer];

                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    const void*  dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                    const float* buf     = static_cast<const float*>(dataPtr);
                    for(size_t index = 0; index < accessor.count; index++)
                    {
                        sampler.inputs.push_back(buf[index]);
                    }

                    for(auto input : sampler.inputs)
                    {
                        if(input < animation.start)
                        {
                            animation.start = input;
                        };
                        if(input > animation.end)
                        {
                            animation.end = input;
                        }
                    }
                }

                // Read sampler output T/R/S values
                {
                    const tinygltf::Accessor&   accessor   = gltfModel.accessors[samp.output];
                    const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer&     buffer     = gltfModel.buffers[bufferView.buffer];

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

                animation.samplers.push_back(sampler);
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
                channel.node         = GetNodeByIndex(source.target_node);
                if(!channel.node)
                {
                    continue;
                }

                animation.channels.push_back(channel);
            }

            animations.push_back(animation);
        }
    }

    void Scene::getSceneDimensions()
    {
        // Calculate binary volume hierarchy for all nodes in the scene
        for(auto& node : linearNodes)
        {
            calculateBoundingBox(node.get(), nullptr);
        }

        dimensions.min = glm::vec3(FLT_MAX);
        dimensions.max = glm::vec3(-FLT_MAX);

        for(auto& node : linearNodes)
        {
            if(node->bvh.valid)
            {
                dimensions.min = glm::min(dimensions.min, node->bvh.min);
                dimensions.max = glm::max(dimensions.max, node->bvh.max);
            }
        }

        // Calculate scene aabb
        aabb       = glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]));
        aabb[3][0] = dimensions.min[0];
        aabb[3][1] = dimensions.min[1];
        aabb[3][2] = dimensions.min[2];
    }

    void Scene::calculateBoundingBox(Node* node, Node* parent)
    {
        BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

        if(node->mesh)
        {
            if(node->mesh->bb.valid)
            {
                node->aabb = node->mesh->bb.getAABB(node->getMatrix());
                if(node->children.size() == 0)
                {
                    node->bvh.min   = node->aabb.min;
                    node->bvh.max   = node->aabb.max;
                    node->bvh.valid = true;
                }
            }
        }

        parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
        parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

        for(auto& child : node->children)
        {
            calculateBoundingBox(child, node);
        }
    }


    void Scene::AssertSceneloaded(bool loaded)
    {
        bool isLoaded = false;
        if(loaded != isLoaded)
        {
            throw Exception("Assertion failed: Call executed expecting scene to be {}, but scene was {}!", (loaded ? "loaded" : "unloaded"), (isLoaded ? "loaded" : "unloaded"));
        }
    }
}  // namespace hsk