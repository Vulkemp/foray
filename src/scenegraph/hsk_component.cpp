#include "hsk_component.hpp"
#include "hsk_node.hpp"
#include "hsk_registry.hpp"
#include "hsk_scene.hpp"

namespace hsk {
    NNode*    NodeComponent::GetNode() { return dynamic_cast<NNode*>(mRegistry); }
    NScene*   NodeComponent::GetScene() { return dynamic_cast<NScene*>(mRegistry->GetRootRegistry()); }
    Registry* NodeComponent::GetGlobals() { return dynamic_cast<Registry*>(mRegistry->GetRootRegistry()); }
    const VkContext* NodeComponent::GetContext() {
        
    }
    NScene*   GlobalComponent::GetScene() { return dynamic_cast<NScene*>(mRegistry); }
    Registry* GlobalComponent::GetGlobals() { return mRegistry; }
    const VkContext* GlobalComponent::GetContext() {}
}  // namespace hsk