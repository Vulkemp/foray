#pragma once
namespace foray::scene {
    class Registry;
    class CallbackDispatcher;
    class Component;
    class Node;
    class Scene;
    class Mesh;
    class Primitive;
    namespace ncomp {
        class Camera;
        class Transform;
        class MeshInstance;
        class PunctualLight;
    }  // namespace ncomp
    namespace gcomp {
        class GeometryStore;
        class MaterialManager;
        class TextureManager;
        class AnimationManager;
    }  // namespace gcomp
    struct SceneDrawInfo;
    class Animation;
    struct AnimationSampler;
    struct AnimationChannel;
    struct PlaybackConfig;
    struct CameraUboBlock;
}  // namespace foray::scene