#pragma once
#include "../osi/hsk_osi_declares.hpp"
#include "hsk_registry.hpp"
#include "hsk_rootregistry.hpp"
#include "hsk_scenedrawing.hpp"
#include "hsk_scenegraph_declares.hpp"

namespace hsk {

    class NScene : public RootRegistry
    {
      public:
        explicit NScene(const VkContext* context);


        /// @brief Generates a new node and attaches it to the parent if it is set, root otherwise
        NNode* MakeNode(NNode* parent = nullptr);

        void Update(const FrameUpdateInfo& updateInfo);
        void Draw(const FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout);
        void HandleEvent(std::shared_ptr<Event>& event);

        /// @brief Registry of Global Components. Global Components' callbacks are invoked after all node-attached components
        HSK_PROPERTY_GET(Globals)

        void Cleanup();

        inline virtual ~NScene() { Cleanup(); }

      protected:
        const VkContext* mContext;
        /// @brief Buffer holding ownership of all nodes
        /// @remark Holds unique ptrs to be able to preserve pointers if the buffer is changed or moved. These pointers may be null-equivalent.
        std::vector<std::unique_ptr<NNode>> mNodeBuffer;

        /// @brief All nodes directly attached to the root
        std::vector<NNode*> mRootNodes;

        Registry     mGlobals;
        RootRegistry mGlobalRootRegistry;
    };
}  // namespace hsk