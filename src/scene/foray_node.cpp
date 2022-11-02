#include "foray_node.hpp"
#include "components/foray_transform.hpp"
#include "foray_scene.hpp"

namespace foray::scene {
    ncomp::Transform* Node::GetTransform()
    {
        return GetComponent<ncomp::Transform>();
    }

    Node::Node(Scene* scene, Node* parent) : Registry(scene), mParent(parent)
    {
        MakeComponent<ncomp::Transform>();
    }
}  // namespace foray