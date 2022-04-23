#include "hsk_scenecomponent.hpp"
#include "hsk_scene.hpp"

namespace hsk {

    SceneComponent::SceneComponent(Scene* scene) : mScene(scene) {}

    SceneVkContext*       SceneComponent::Context() { return &(mScene->Context()); }
    const SceneVkContext* SceneComponent::Context() const { return &(mScene->Context()); }
}  // namespace hsk