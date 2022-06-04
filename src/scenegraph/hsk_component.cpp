#include "hsk_component.hpp"
#include "hsk_registry.hpp"
#include "hsk_scene.hpp"
#include "hsk_node.hpp"

namespace hsk {
    NNode*  Component::GetNode() { return dynamic_cast<NNode*>(mRegistry); }
    NScene* Component::GetScene() { return dynamic_cast<NScene*>(mRegistry->GetRootRegistry()); }
}  // namespace hsk