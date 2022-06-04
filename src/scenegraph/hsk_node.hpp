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

      protected:

        NNode*              mParent   = nullptr;
        std::vector<NNode*> mChildren = {};
    };
}  // namespace hsk