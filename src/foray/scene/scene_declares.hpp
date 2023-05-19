#pragma once
#include "../osi/osi_declares.hpp"

namespace foray::scene {
    class Registry;
    class CallbackDispatcher;
    class Component;
    class Node;
    class Scene;
    class Mesh;
    struct Primitive;
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
    struct SceneUpdateInfo;
    struct SceneDrawInfo;
    class Animation;
    struct AnimationSampler;
    struct AnimationChannel;
    struct PlaybackConfig;
    struct CameraUboBlock;

    using TUpdateMessage  = SceneUpdateInfo&;
    using TDrawMessage    = SceneDrawInfo&;
    using TOsEventMessage = const osi::Event*;
}  // namespace foray::scene