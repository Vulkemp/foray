#include "hsk_node.hpp"
#include "components/hsk_transform.hpp"
#include "hsk_scene.hpp"

namespace hsk {
    NTransform* NNode::GetTransform()
    {
        NTransform* transform = GetComponent<NTransform>();
        if(!transform)
        {
            transform = MakeComponent<NTransform>();
        }
        return transform;
    }

    NNode::NNode(NScene* scene, NNode* parent) : Registry(scene), mParent(parent)
    {
        MakeComponent<NTransform>();
        if(parent)
        {
            parent->GetChildren().push_back(this);
        }
    }
}  // namespace hsk