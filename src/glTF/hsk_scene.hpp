#pragma once
#include "hsk_Material.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_texture.hpp"
#include <memory>
#include <tinygltf/tiny_gltf.h>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace hsk {

    class Scene
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

      protected:
        VkContext                             mContext;
        std::vector<Material>                 mMaterials;
        std::vector<std::unique_ptr<Texture>> mTextures;


        void AssertSceneloaded(bool loaded = true);
    };
}  // namespace hsk