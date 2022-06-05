#include "hsk_component.hpp"
#include "hsk_node.hpp"
#include "hsk_registry.hpp"
#include "hsk_scene.hpp"

namespace hsk {
    NNode*    Component::GetNode() { return dynamic_cast<NNode*>(mRegistry); }
    NScene*   Component::GetScene() { return dynamic_cast<NScene*>(mRegistry->GetRootRegistry()); }
    Registry* Component::GetGlobals()
    {
        auto scene = GetScene();
        if(scene)
        {
            return &(scene->GetGlobals());
        }
        else
        {
            return nullptr;
        }
    }
}  // namespace hsk