#pragma once
#include "hsk_animation.hpp"
#include "hsk_geo.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_material.hpp"
#include "hsk_skin.hpp"
#include "hsk_texture.hpp"
#include <memory>
#include <tinygltf/tiny_gltf.h>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "hsk_node.hpp"

namespace hsk {

    class Scene : public NoMoveDefaults
    {
      public:
        struct VkContext
        {
            VmaAllocator     Allocator           = nullptr;
            VkDevice         Device              = nullptr;
            VkPhysicalDevice PhysicalDevice      = nullptr;
            VkCommandPool    TransferCommandPool = nullptr;
            VkQueue          TransferQueue       = nullptr;
        };

        inline Scene& Context(VkContext& context)
        {
            mContext = context;
            return *this;
        }
        inline VkContext&       Context() { return mContext; }
        inline const VkContext& Context() const { return mContext; }

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

        void destroy();

        void loadFromFile(std::string filename, float scale = 1.f);

        inline std::vector<Material>& Materials() { return mMaterials; }

        struct Dimensions
        {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
        } dimensions;

      protected:
        VkContext                             mContext                = {};
        std::vector<Material>                 mMaterials              = {};
        std::vector<std::unique_ptr<Texture>> mTextures               = {};
        glm::mat4                             mAxisAlignedBoundingBox = {};

        std::vector<Node*>                 mNodesHierarchy = {};
        std::vector<std::unique_ptr<Node>> mNodesLinear    = {};

        std::vector<std::unique_ptr<Skin>> mSkins = {};

        VertexBuffer vertices = {};
        IndexBuffer  indices  = {};

        std::vector<TextureSampler> mTextureSamplers = {};
        std::vector<Animation>      mAnimations      = {};
        std::vector<std::string>    mExtensions      = {};

        void AssertSceneloaded(bool loaded = true);

        void loadTextureSamplers(const tinygltf::Model& gltfModel);
        void loadTextures(const tinygltf::Model& gltfModel);
        void loadMaterials(const tinygltf::Model& gltfModel);
        void loadSkins(const tinygltf::Model& gltfModel);
        void loadAnimations(const tinygltf::Model& gltfModel);
        void LoadNodeRecursive(const tinygltf::Model& gltfModel, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

        void calculateSceneDimensions();
        void calculateBoundingBox(Node* node, Node* parent);
    };
}  // namespace hsk