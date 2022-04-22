#include "hsk_scene.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../hsk_vmaHelper.hpp"
#include "hsk_animation.hpp"
#include "hsk_mesh.hpp"
#include "hsk_node.hpp"
#include "hsk_skin.hpp"
#include "hsk_texture.hpp"

namespace hsk {
    void Scene::destroy()
    {
        if(vertices.Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(mContext.Allocator, vertices.Buffer, vertices.Allocation);
            vertices.Buffer = VK_NULL_HANDLE;
        }
        if(indices.Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(mContext.Allocator, indices.Buffer, indices.Allocation);
            indices.Buffer = VK_NULL_HANDLE;
        }

        // Texture has a destructor which cleans up unmanaged buffers automatically, so this is safe
        mTextures.resize(0);

        // Texturesamplers are dumb structs, so this is safe
        mTextureSamplers.resize(0);

        // Hierarchy only contains non-owning references. Nodes have proper destructors and linear nodes are std::unique_ptr, so this is safe
        mNodesHierarchy.resize(0);
        mNodesLinear.resize(0);

        // Materials are dumb structs, so this is safe
        mMaterials.resize(0);

        // Animations are dumb structs, so this is safe
        mAnimations.resize(0);

        mExtensions.resize(0);

        // TODO: Kill skins
        mSkins.resize(0);
    }

    void Scene::LoadNodeRecursive(const tinygltf::Model& gltfModel, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
    {
        const tinygltf::Node& gltfnode = gltfModel.nodes[index];
        std::unique_ptr<Node> node     = std::make_unique<Node>(this);
        node->InitFromTinyGltfNode(gltfModel, gltfnode, index, indexBuffer, vertexBuffer);
        mNodesLinear[index] = std::move(node);

        for(int32_t childIndex : gltfnode.children)
        {
            LoadNodeRecursive(gltfModel, childIndex, indexBuffer, vertexBuffer);
        }
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

            // Gltf files can contain multiple scenes, which are maintained in one large node array.
            // Solution here is to always have the entire vector ready so node indices can be resolved efficiently.
            // our storage vector contains std::unique_ptr, which are small. Most models will only contain one scene.
            mNodesLinear.resize(gltfModel.nodes.size());

            // We load nodes recursively, as this is the only way to catch all nodes that are part of our scene
            for(size_t i = 0; i < scene.nodes.size(); i++)
            {
                LoadNodeRecursive(gltfModel, scene.nodes[i], indexBuffer, vertexBuffer);
            }

            // Setup parents
            for(auto& node : mNodesLinear)
            {
                if(node)
                {
                    node->ResolveParent();
                    if(!node->parent)
                    {
                        mNodesHierarchy.push_back(node.get());
                    }
                }
            }

            // TODO: Load Animations
            if(gltfModel.animations.size() > 0)
            {
                loadAnimations(gltfModel);
            }
            loadSkins(gltfModel);

            for(auto& node : mNodesLinear)
            {
                // Assign skins
                if(node->skinIndex > -1)
                {
                    node->skin = mSkins[node->skinIndex].get();
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

        mExtensions = gltfModel.extensionsUsed;

        size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
        size_t indexBufferSize  = indexBuffer.size() * sizeof(uint32_t);
        indices.Count           = static_cast<uint32_t>(indexBuffer.size());

        assert(vertexBufferSize > 0);

        struct StagingBuffer
        {
            VkBuffer      buffer;
            VmaAllocation allocation;
        } vertexStaging, indexStaging;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.flags                   = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
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
        createBuffer(mContext.Allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo, &vertices.Allocation, vertexBufferSize, &vertices.Buffer);
        // Index buffer
        if(indexBufferSize > 0)
        {
            createBuffer(mContext.Allocator, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo, &indices.Allocation, indexBufferSize, &indices.Buffer);
        }

        // Copy from staging buffers
        VkCommandBuffer copyCmd = createCommandBuffer(mContext.Device, mContext.TransferCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copyRegion = {};

        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.Buffer, 1, &copyRegion);

        if(indexBufferSize > 0)
        {
            copyRegion.size = indexBufferSize;
            vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.Buffer, 1, &copyRegion);
        }

        flushCommandBuffer(mContext.Device, mContext.TransferCommandPool, copyCmd, mContext.TransferQueue, true);

        vmaDestroyBuffer(mContext.Allocator, vertexStaging.buffer, vertexStaging.allocation);
        if(indexBufferSize > 0)
        {
            vmaDestroyBuffer(mContext.Allocator, indexStaging.buffer, indexStaging.allocation);
        }

        calculateSceneDimensions();
    }

    void Scene::loadTextureSamplers(const tinygltf::Model& gltfModel)
    {
        for(const tinygltf::Sampler& smpl : gltfModel.samplers)
        {
            TextureSampler sampler{};
            sampler.InitFromTinyGltfSampler(smpl);
            mTextureSamplers.push_back(sampler);
        }
    }

    void Scene::loadTextures(const tinygltf::Model& gltfModel)
    {
        for(const tinygltf::Texture& tex : gltfModel.textures)
        {
            tinygltf::Image image = gltfModel.images[tex.source];
            TextureSampler  textureSampler;
            if(tex.sampler >= 0)
            {
                textureSampler = mTextureSamplers[tex.sampler];
            }
            std::unique_ptr<Texture> texture = std::make_unique<Texture>(this);
            texture->InitFromTinyGltfImage(image, textureSampler);
            mTextures.push_back(std::move(texture));
        }
    }

    void Scene::loadMaterials(const tinygltf::Model& gltfModel)
    {
        for(auto& gltfmat : gltfModel.materials)
        {
            Material material(this);
            material.InitFromTinyGltfMaterial(gltfmat);
            mMaterials.push_back(material);
        }
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

            mSkins.push_back(std::move(newSkin));
        }
    }

    void Scene::loadAnimations(const tinygltf::Model& gltfModel)
    {
        for(int32_t i = 0; i < gltfModel.animations.size(); i++)
        {
            std::unique_ptr<Animation> animation = std::make_unique<Animation>(this);
            animation->InitFromTinyGltfAnimation(gltfModel, gltfModel.animations[i], i);
            mAnimations.push_back(std::move(animation));
        }
    }

    void Scene::calculateSceneDimensions()
    {
        // Calculate binary volume hierarchy for all nodes in the scene
        for(auto& node : mNodesLinear)
        {
            calculateBoundingBox(node.get(), nullptr);
        }

        dimensions.min = glm::vec3(FLT_MAX);
        dimensions.max = glm::vec3(-FLT_MAX);

        for(auto& node : mNodesLinear)
        {
            if(node->bvh.valid)
            {
                dimensions.min = glm::min(dimensions.min, node->bvh.min);
                dimensions.max = glm::max(dimensions.max, node->bvh.max);
            }
        }

        // Calculate scene aabb
        mAxisAlignedBoundingBox =
            glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]));
        mAxisAlignedBoundingBox[3][0] = dimensions.min[0];
        mAxisAlignedBoundingBox[3][1] = dimensions.min[1];
        mAxisAlignedBoundingBox[3][2] = dimensions.min[2];
    }

    void Scene::calculateBoundingBox(Node* node, Node* parent)
    {
        BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

        if(node->mesh)
        {
            if(node->mesh->mBoundingBox.valid)
            {
                node->aabb = node->mesh->mBoundingBox.getAABB(node->getMatrix());
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

    void Scene::updateAnimation(uint32_t index, float time)
    {
        if(mAnimations.empty())
        {
            // std::cout << ".glTF does not contain animation." << std::endl;
            return;
        }
        if(index > static_cast<uint32_t>(mAnimations.size()) - 1)
        {
            // std::cout << "No animation with index " << index << std::endl;
            return;
        }
        Animation* animation = mAnimations[index].get();

        bool updated = animation->Update(time);
        if(updated)
        {
            for(auto& node : mNodesLinear)
            {
                if(node)
                {
                    node->update();
                }
            }
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