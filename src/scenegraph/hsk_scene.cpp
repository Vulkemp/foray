#include "hsk_scene.hpp"
#include "hsk_node.hpp"

namespace hsk {
    NNode* NScene::MakeNode(NNode* parent)
    {
        auto nodeManagedPtr = std::make_unique<NNode>(this, parent);
        auto node           = nodeManagedPtr.get();
        mNodeBuffer.push_back(std::move(nodeManagedPtr));
        if(!parent)
        {
            mRootNodes.push_back(node);
        }
        return node;
    }
}  // namespace hsk