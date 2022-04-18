#pragma once
#include "hsk_glTF_declares.hpp"
#include <tinygltf/tiny_gltf.h>
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

      protected:
        VkContext mContext;

        void AssertSceneloaded(bool loaded = true);
    };
}  // namespace hsk