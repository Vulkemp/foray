#include "hsk_node.hpp"
#include "components/hsk_transform.hpp"
#include "hsk_scene.hpp"

namespace hsk {
    Transform* Node::GetTransform()
    {
        return GetComponent<Transform>();
    }

    Node::Node(Scene* scene, Node* parent) : Registry(scene), mParent(parent)
    {
        MakeComponent<Transform>();
        if(parent)
        {
            parent->GetChildren().push_back(this);
        }
    }
}  // namespace hsk