#include "foray_component.hpp"
#include "foray_node.hpp"
#include "foray_registry.hpp"
#include "foray_scene.hpp"

namespace foray::scene {
    Node*           NodeComponent::GetNode() { return dynamic_cast<Node*>(mRegistry); }
    Scene*          NodeComponent::GetScene() { return dynamic_cast<Scene*>(mRegistry->GetCallbackDispatcher()); }
    Registry*        NodeComponent::GetGlobals() { return dynamic_cast<Registry*>(mRegistry->GetCallbackDispatcher()); }
    core::Context* NodeComponent::GetContext()
    {
        auto scene = GetScene();
        if(scene)
        {
            return scene->GetContext();
        }
        return nullptr;
    }
    Scene*          GlobalComponent::GetScene() { return dynamic_cast<Scene*>(mRegistry); }
    Registry*        GlobalComponent::GetGlobals() { return mRegistry; }
    core::Context* GlobalComponent::GetContext()
    {
        auto scene = GetScene();
        if(scene)
        {
            return scene->GetContext();
        }
        return nullptr;
    }
}  // namespace foray