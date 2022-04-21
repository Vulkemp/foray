#pragma once
#include "hsk_geo.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_material.hpp"
#include "hsk_texture.hpp"
#include <memory>
#include <tinygltf/tiny_gltf.h>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {

    class Scene : public NoMoveDefaults
    {
      public:
        struct VkContext
        {
            VmaAllocator     Allocator;
            VkDevice         Device;
            VkPhysicalDevice PhysicalDevice;
            VkCommandPool    TransferCommandPool;
            VkQueue          TransferQueue;
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
            if(index >= 0 && index < linearNodes.size())
            {
                return linearNodes[index].get();
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
        VkContext                             mContext;
        std::vector<Material>                 mMaterials;
        std::vector<std::unique_ptr<Texture>> mTextures;
        glm::mat4                             aabb;

        std::vector<Node*>                 nodes;
        std::vector<std::unique_ptr<Node>> linearNodes;

        std::vector<std::unique_ptr<Skin>> skins;

        Vertices vertices;
        Indices  indices;

        std::vector<TextureSampler> textureSamplers;
        std::vector<Animation>      animations;
        std::vector<std::string> extensions;

        void AssertSceneloaded(bool loaded = true);

        void loadTextureSamplers(tinygltf::Model& gltfModel);
        void loadTextures(tinygltf::Model& gltfModel);
        void loadMaterials(tinygltf::Model& gltfModel);
        void loadNode(Node*                  parent,
                      const tinygltf::Node&  node,
                      uint32_t               nodeIndex,
                      const tinygltf::Model& model,
                      std::vector<uint32_t>& indexBuffer,
                      std::vector<Vertex>&   vertexBuffer,
                      float                  globalscale);
        void loadSkins(const tinygltf::Model& gltfModel);
        void loadAnimations(tinygltf::Model& gltfModel);

        void getSceneDimensions();
        void calculateBoundingBox(Node* node, Node* parent);
    };
}  // namespace hsk