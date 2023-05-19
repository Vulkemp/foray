#include "node.hpp"
#include "components/transform.hpp"
#include "scene.hpp"

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