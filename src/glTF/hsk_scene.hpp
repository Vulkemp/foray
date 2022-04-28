#pragma once
#include "../hsk_vmaHelpers.hpp"
#include "hsk_animation.hpp"
#include "hsk_geo.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_material.hpp"
#include "hsk_node.hpp"
#include "hsk_scenecomponent.hpp"
#include "hsk_skin.hpp"
#include "hsk_texture.hpp"
#include <memory>
#include <tinygltf/tiny_gltf.h>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "hsk_camera.hpp"

namespace hsk {

    class Scene : public NoMoveDefaults
    {
      public:
        Scene(VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool transferpool, VkQueue transferqueue);

        inline Scene& Context(SceneVkContext& context)
        {
            mContext = context;
            return *this;
        }
        inline SceneVkContext&       Context() { return mContext; }
        inline const SceneVkContext& Context() const { return mContext; }

        inline void GetTextures(std::vector<Texture*>& out)
        {
            out.reserve(mTextures.size());
            for(auto& tex : mTextures)
            {
                out.push_back(tex.get());
            }
        }

        inline Texture* GetTextureByIndex(int32_t index)
        {
            if(index >= 0 && index < mTextures.size())
            {
                return mTextures[index].get();
            }
            return nullptr;
        }

        inline Node* GetNodeByIndex(int32_t index)
        {
            if(index >= 0 && index < mNodesLinear.size())
            {
                return mNodesLinear[index].get();
            }
            return nullptr;
        }

        void Cleanup();

        void LoadFromFile(std::string filename, float scale = 1.f);

        inline std::vector<Material>& Materials() { return mMaterials; }

        struct Dimensions
        {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
        } dimensions;

        void Draw(VkCommandBuffer cmdbuffer);

        virtual ~Scene();

        HSK_PROPERTY_ALL(FallbackMaterial)

      protected:
        SceneVkContext                        mContext                = {};
        Material                              mFallbackMaterial        = {};
        std::vector<Material>                 mMaterials              = {};
        std::vector<std::unique_ptr<Texture>> mTextures               = {};
        glm::mat4                             mAxisAlignedBoundingBox = {};

        std::vector<Node*>                 mNodesHierarchy = {};
        std::vector<std::unique_ptr<Node>> mNodesLinear    = {};

        std::vector<std::unique_ptr<Skin>> mSkins = {};

        ManagedBuffer vertices = {};
        ManagedBuffer indices  = {};

        std::vector<TextureSampler>             mTextureSamplers = {};
        std::vector<std::unique_ptr<Animation>> mAnimations      = {};
        std::vector<std::string>                mExtensions      = {};
        std::vector<std::unique_ptr<Camera>> mCameras = {};

        void AssertSceneloaded(bool loaded = true);

        void loadTextureSamplers(const tinygltf::Model& gltfModel);
        void loadTextures(const tinygltf::Model& gltfModel);
        void loadMaterials(const tinygltf::Model& gltfModel);
        void loadSkins(const tinygltf::Model& gltfModel);
        void loadAnimations(const tinygltf::Model& gltfModel);
        void LoadNodeRecursive(const tinygltf::Model& gltfModel, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

        void calculateSceneDimensions();
        void calculateBoundingBox(Node* node, Node* parent);

        void updateAnimation(uint32_t index, float time);
        void drawNode(Node* node, VkCommandBuffer commandBuffer);
    };
}  // namespace hsk