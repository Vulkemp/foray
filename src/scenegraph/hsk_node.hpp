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

        // using iterator       = std::vector<NNode*>::iterator;
        // using const_iterator = std::vector<NNode*>::const_iterator;
        // iterator       begin() noexcept { return mChildren.begin(); }
        // iterator       end() noexcept { return mChildren.end(); }
        // const_iterator cbegin() const noexcept { return mChildren.cbegin(); }
        // const_iterator cend() const noexcept { return mChildren.cend(); }

      protected:

        NNode*              mParent   = nullptr;
        std::vector<NNode*> mChildren = {};
    };
}  // namespace hsk