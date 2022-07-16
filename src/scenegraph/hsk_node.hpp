#pragma once
#include "hsk_registry.hpp"

namespace hsk {
    class Node : public Registry
    {
      public:
        Node(Scene* scene, Node* parent = nullptr);

        HSK_PROPERTY_ALL(Parent);
        HSK_PROPERTY_ALL(Children);

        Transform* GetTransform();

        template <typename TComponent>
        inline int32_t FindChildrenWithComponent(std::vector<Node*>& outnodes);

        inline virtual ~Node(){}

      protected:
        Node*              mParent   = nullptr;
        std::vector<Node*> mChildren = {};
    };


    template <typename TComponent>
    inline int32_t Node::FindChildrenWithComponent(std::vector<Node*>& outnodes){
      int32_t found = 0;
      for (Node* child : mChildren){
        if (child->HasComponent<TComponent>()){
          found++;
          outnodes.push_back(child);
        }
        found += child->FindChildrenWithComponent<TComponent>(outnodes);
      }
      return found;
    }

}  // namespace hsk