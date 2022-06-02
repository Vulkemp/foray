#include "hsk_scene.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../memory/hsk_vmaHelpers.hpp"
#include "hsk_animation.hpp"
#include "hsk_mesh.hpp"
#include "hsk_node.hpp"
#include "hsk_texture.hpp"
#include "hsk_scenedrawinfo.hpp"

namespace hsk {

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Scene::GetTextureDescriptorInfo()
    {

        auto descriptorInfo                = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->ShaderStageFlags   = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptorInfo->pImmutableSamplers = nullptr;
        descriptorInfo->DescriptorCount    = mTextures.size();
        descriptorInfo->DescriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        size_t numSets = 1;
        descriptorInfo->ImageInfos.resize(numSets);

        for(size_t setIndex = 0; setIndex < numSets; setIndex++)
        {
            descriptorInfo->ImageInfos[setIndex].resize(mTextures.size());
            for(size_t i = 0; i < mTextures.size(); i++)
            {
                descriptorInfo->ImageInfos[setIndex][i].imageLayout = mTextures[i]->GetImageLayout();
                descriptorInfo->ImageInfos[setIndex][i].imageView   = mTextures[i]->GetImageView();
                descriptorInfo->ImageInfos[setIndex][i].sampler =
                    mTextures[i]->GetSampler();  // TODO: whats difference between a sampler of this texture object and the mTextureSamplers - which one should be used??
            }
        }
        return descriptorInfo;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> Scene::GetMaterialUboArrayDescriptorInfo()
    {
        size_t numMaterials = 1;  // we load the complete ubo buffer as a single ubo buffer.

        auto descriptorInfo                = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->ShaderStageFlags   = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptorInfo->pImmutableSamplers = nullptr;
        descriptorInfo->DescriptorCount    = numMaterials;
        descriptorInfo->DescriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        size_t numSets = 1;
        descriptorInfo->BufferInfos.resize(numSets);

        for(size_t setIndex = 0; setIndex < numSets; setIndex++)
        {
            descriptorInfo->BufferInfos[setIndex].resize(numMaterials);
            for(size_t i = 0; i < numMaterials; i++)
            {
                descriptorInfo->BufferInfos[setIndex][i] = mMaterials.GetVkDescriptorBufferInfo();
            }
        }
        return descriptorInfo;
    }

    void Scene::Cleanup()
    {
        vertices.Destroy();
        indices.Destroy();

        // Texture has a destructor which cleans up unmanaged buffers automatically, so this is safe
        mTextures.resize(0);

        // Texturesamplers are dumb structs, so this is safe
        mTextureSamplers.resize(0);

        // Hierarchy only contains non-owning references. Nodes have proper destructors and linear nodes are std::unique_ptr, so this is safe
        mNodesHierarchy.resize(0);
        mNodesLinear.resize(0);

        // Materials are dumb structs, so this is safe
        mMaterials.Cleanup();

        // Animations are dumb structs, so this is safe
        mAnimations.resize(0);

        mExtensions.resize(0);

        // TODO: Kill skins
        mCameras.resize(0);
    }

    Scene::~Scene() { Cleanup(); }

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

    void Scene::LoadFromFile(std::string filename, float scale)
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
                    if(!node->GetParent())
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

            for(auto& node : mNodesLinear)
            {
                // Initial pose
                if(node->GetMesh())
                {
                    node->update();
                }
            }

            for(const auto& cam : gltfModel.cameras)
            {
                std::unique_ptr<Camera> camera = std::make_unique<Camera>(this);
                camera->InitFromTinyGltfCamera(cam);
                mCameras.push_back(std::move(camera));
            }
        }
        else
        {
            HSK_THROWFMT("Could not load gltf file: {}", error);
        }

        mExtensions = gltfModel.extensionsUsed;

        size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
        size_t indexBufferSize  = indexBuffer.size() * sizeof(uint32_t);
        // indices.Count           = static_cast<uint32_t>(indexBuffer.size());

        Assert(vertexBufferSize > 0);


        // Create Vertex and Index buffer (and stage + transfer data to device)

        vertices.SetName("Vertices");
        vertices.Create(mContext, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertexBufferSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        vertices.WriteDataDeviceLocal(vertexBuffer.data(), vertexBufferSize);

        // Index buffer
        if(indexBufferSize > 0)
        {
            indices.SetName("Indices");
            indices.Create(mContext, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indexBufferSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
            indices.WriteDataDeviceLocal(indexBuffer.data(), indexBufferSize);
        }

        calculateSceneDimensions();
        mSceneLoaded = true;
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
        // mMaterials.Context() = this->Context();
        mMaterials.InitFromTinyGltfMaterials(gltfModel.materials);
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
            if(node->GetBvh().GetValid())
            {
                dimensions.min = glm::min(dimensions.min, node->GetBvh().GetMin());
                dimensions.max = glm::max(dimensions.max, node->GetBvh().GetMax());
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
        BoundingBox parentBvh = parent ? parent->GetBvh() : BoundingBox(dimensions.min, dimensions.max);

        if(node->GetMesh())
        {
            if(node->GetMesh()->GetBounds().GetValid())
            {
                node->GetAxisAlignedBoundingBox() = node->GetMesh()->GetBounds().getAABB(node->getMatrix());
                if(node->GetChildren().size() == 0)
                {
                    node->GetBvh().SetMin(node->GetAxisAlignedBoundingBox().GetMin());
                    node->GetBvh().SetMax(node->GetAxisAlignedBoundingBox().GetMax());
                    node->GetBvh().SetValid(true);
                }
            }
        }

        parentBvh.SetMin(glm::min(parentBvh.GetMin(), node->GetBvh().GetMin()));
        parentBvh.SetMax(glm::min(parentBvh.GetMax(), node->GetBvh().GetMax()));

        for(auto& child : node->GetChildren())
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
        if(loaded != mSceneLoaded)
        {
            HSK_THROWFMT("Scene expected to be {}, but scene was {}!", (loaded ? "loaded" : "unloaded"), (mSceneLoaded ? "loaded" : "unloaded"));
        }
    }

    void Scene::Draw(SceneDrawInfo& drawInfo)
    {
        if(!vertices.GetAllocation())
        {
            return;
        }
        const VkDeviceSize offsets[1]      = {0};
        VkBuffer           vertexBuffers[] = {vertices.GetBuffer()};
        vkCmdBindVertexBuffers(drawInfo.CmdBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(drawInfo.CmdBuffer, indices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        for(auto node : mNodesHierarchy)
        {
            node->Draw(drawInfo);
        }
    }

    void Scene::drawNode(Node* node, VkCommandBuffer commandBuffer)
    {
        auto& mesh = node->GetMesh();
        if(mesh)
        {
            // TODO: Bind desired descriptor sets
            //vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSets[i], 0, nullptr);
            for(const auto& primitive : node->GetMesh()->GetPrimitives())
            {
                vkCmdDrawIndexed(commandBuffer, primitive->IndexCount, 1, primitive->FirstIndex, 0, 0);
            }
        }
        for(auto& child : node->GetChildren())
        {
            drawNode(child, commandBuffer);
        }
    }
}  // namespace hsk