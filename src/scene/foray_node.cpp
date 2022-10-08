#include "foray_node.hpp"
#include "components/foray_transform.hpp"
#include "foray_scene.hpp"

namespace foray::scene {
    Transform* Node::GetTransform()
    {
        return GetComponent<Transform>();
    }

    Node::Node(Scene* scene, Node* parent) : Registry(scene), mParent(parent)
    {
        MakeComponent<Transform>();
    }
}  // namespace foray