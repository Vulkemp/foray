#pragma once
#include "hsk_glTF_declares.hpp"
#include <vma/vk_mem_alloc.h>

namespace hsk {
    struct SceneVkContext
    {
        VmaAllocator     Allocator           = nullptr;
        VkDevice         Device              = nullptr;
        VkPhysicalDevice PhysicalDevice      = nullptr;
        VkCommandPool    TransferCommandPool = nullptr;
        VkQueue          TransferQueue       = nullptr;
    };

    struct SceneComponent
    {
      public:
        inline SceneComponent() {}
        explicit SceneComponent(Scene* scene);

        SceneVkContext*       Context();
        const SceneVkContext* Context() const;

        inline Scene*       Owner() { return mScene; }
        inline const Scene* Owner() const { return mScene; }

      protected:
        Scene* mScene = {};
    };

}  // namespace hsk