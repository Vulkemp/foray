#include "hsk_component.hpp"
#include "hsk_node.hpp"
#include "hsk_registry.hpp"
#include "hsk_scene.hpp"

namespace hsk {
    Node*           NodeComponent::GetNode() { return dynamic_cast<Node*>(mRegistry); }
    Scene*          NodeComponent::GetScene() { return dynamic_cast<Scene*>(mRegistry->GetCallbackDispatcher()); }
    Registry*        NodeComponent::GetGlobals() { return dynamic_cast<Registry*>(mRegistry->GetCallbackDispatcher()); }
    const VkContext* NodeComponent::GetContext()
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
    const VkContext* GlobalComponent::GetContext()
    {
        auto scene = GetScene();
        if(scene)
        {
            return scene->GetContext();
        }
        return nullptr;
    }
}  // namespace hsk