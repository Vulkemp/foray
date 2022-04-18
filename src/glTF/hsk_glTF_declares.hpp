#pragma once
#include "../hsk_basics.hpp"

namespace hsk {
    class Scene;
    class Texture;
    class TextureSampler;
    class Node;

    class SceneComponent
    {
      public:
        inline SceneComponent() {}
        inline explicit SceneComponent(Scene* scene) : mOwningScene(scene) {}
        inline virtual ~SceneComponent() {}

        inline void         OwningScene(Scene* scene) { mOwningScene = scene; }
        inline Scene*       OwningScene() { return mOwningScene; }
        inline const Scene* OwningScene() const { return mOwningScene; }

      protected:
        Scene* mOwningScene = nullptr;
    };
}  // namespace hsk
