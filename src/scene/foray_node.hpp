#pragma once
#include "foray_registry.hpp"

namespace foray::scene {
    class Node : public Registry
    {
      public:
        Node(Scene* scene, Node* parent = nullptr);

        FORAY_PROPERTY_V(Parent);
        FORAY_PROPERTY_R(Children);
        FORAY_PROPERTY_R(Name);

        ncomp::Transform* GetTransform();

        template <typename TComponent>
        inline int32_t FindChildrenWithComponent(std::vector<Node*>& outnodes);

        template <typename TComponent>
        inline int32_t FindComponentsRecursive(std::vector<TComponent*>& outnodes);

        inline virtual ~Node() {}

      protected:
        Node*              mParent   = nullptr;
        std::vector<Node*> mChildren = {};
        std::string mName = "";
    };


    template <typename TComponent>
    inline int32_t Node::FindChildrenWithComponent(std::vector<Node*>& outnodes)
    {
        int32_t found = 0;
        for(Node* child : mChildren)
        {
            if(child->HasComponent<TComponent>())
            {
                found++;
                outnodes.push_back(child);
            }
            found += child->FindChildrenWithComponent<TComponent>(outnodes);
        }
        return found;
    }

    template <typename TComponent>
    inline int32_t Node::FindComponentsRecursive(std::vector<TComponent*>& outnodes)
    {
        int32_t found = 0;
        found += GetComponents<TComponent>(outnodes);
        for(Node* child : mChildren)
        {
            found += child->FindComponentsRecursive<TComponent>(outnodes);
        }
        return found;
    }

}  // namespace foray::scene