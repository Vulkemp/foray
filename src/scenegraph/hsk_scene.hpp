#pragma once
#include "hsk_rootregistry.hpp"
#include "hsk_scenegraph_declares.hpp"

namespace hsk {

    class NScene : public RootRegistry
    {
      public:

        /// @brief Generates a new node and attaches it to the parent if it is set, root otherwise
        NNode* MakeNode(NNode* parent = nullptr);

      protected:
        /// @brief Buffer holding ownership of all nodes
        /// @remark Holds unique ptrs to be able to preserve pointers if the buffer is changed or moved. These pointers may be null-equivalent.
        std::vector<std::unique_ptr<NNode>> mNodeBuffer;

        /// @brief All nodes directly attached to the root
        std::vector<NNode*> mRootNodes;
    };
}  // namespace hsk