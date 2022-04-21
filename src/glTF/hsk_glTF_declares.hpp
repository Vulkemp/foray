#pragma once
#include "../hsk_basics.hpp"

namespace hsk {
    class Scene;
    class Texture;
    class TextureSampler;
    class Node;
    struct Material;
    class Primitive;
    class Mesh;
    class BoundingBox;
    struct Vertex;
    struct Vertices;
    struct Indices;
    class Skin;
    struct AnimationChannel;
    struct AnimationSampler;
    struct Animation;

    struct SceneComponent
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
