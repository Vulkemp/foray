#include "hsk_scenecomponent.hpp"
#include "hsk_scene.hpp"

namespace hsk {

    SceneComponent::SceneComponent(Scene* scene) : mScene(scene) {}

    const VkContext* SceneComponent::Context() { return mScene->Context(); }
}  // namespace hsk