#pragma once
#include "hsk_gltf_declares.hpp"
#include <vma/vk_mem_alloc.h>
#include "../base/hsk_vkcontext.hpp"

namespace hsk {
    struct SceneComponent
    {
      public:
        inline SceneComponent() {}
        explicit SceneComponent(Scene* scene);

        const VkContext* Context();

        inline Scene*       Owner() { return mScene; }
        inline const Scene* Owner() const { return mScene; }

      protected:
        Scene* mScene = {};
    };

}  // namespace hsk