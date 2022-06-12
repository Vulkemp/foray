#pragma once
#include "hsk_registry.hpp"

namespace hsk {
    class NNode : public Registry
    {
      public:
        NNode(NScene* scene, NNode* parent = nullptr);

        HSK_PROPERTY_ALL(Parent);
        HSK_PROPERTY_ALL(Children);

        NTransform* GetTransform();

        template <typename TComponent>
        inline int32_t FindChildrenWithComponent(std::vector<NNode*> outnodes);

      protected:
        NNode*              mParent   = nullptr;
        std::vector<NNode*> mChildren = {};
    };


    template <typename TComponent>
    inline int32_t NNode::FindChildrenWithComponent(std::vector<NNode*> outnodes){
      int32_t found = 0;
      for (NNode* child : mChildren){
        if (child->HasComponent<TComponent>()){
          found++;
          outnodes.push_back(child);
        }
        found += child->FindChildrenWithComponent<TComponent>(outnodes);
      }
      return found;
    }

}  // namespace hsk