#include "hsk_component.hpp"
#include "hsk_node.hpp"
#include "hsk_registry.hpp"
#include "hsk_scene.hpp"

namespace hsk {
    NNode*           NodeComponent::GetNode() { return dynamic_cast<NNode*>(mRegistry); }
    NScene*          NodeComponent::GetScene() { return dynamic_cast<NScene*>(mRegistry->GetCallbackDispatcher()); }
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
    NScene*          GlobalComponent::GetScene() { return dynamic_cast<NScene*>(mRegistry); }
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