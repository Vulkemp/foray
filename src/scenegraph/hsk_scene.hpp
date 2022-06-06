#pragma once
#include "../osi/hsk_osi_declares.hpp"
#include "hsk_registry.hpp"
#include "hsk_rootregistry.hpp"
#include "hsk_scenedrawing.hpp"
#include "hsk_scenegraph_declares.hpp"
#include "hsk_node.hpp"

namespace hsk {

    /// @brief Provides registries and methods as the anchor of a component based scene.
    /// @remark Manages and owns all nodes
    /// @remark Inherits Registry, and functions as the component registry for all global components
    /// @remark Inherits RootRegistry for managing callbacks used by node components, and as a way for nodes to link back to the global instance
    class NScene : public Registry, public RootRegistry
    {
      public:
        explicit NScene(const VkContext* context);


        /// @brief Generates a new node and attaches it to the parent if it is set, root otherwise
        NNode* MakeNode(NNode* parent = nullptr);

        /// @brief Advance scene state by invoking all NodeComponent update callbacks, followed by GlobalComponent update callbacks
        void Update(const FrameUpdateInfo& updateInfo);
        /// @brief Draw the scene by first invoking all BeforeDraw callbacks (NodeComponent, then GlobalComponent), followed by Draw callbacks (NodeComponent, then GlobalComponent).
        void Draw(const FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout);
        /// @brief Invokes event callbacks (NodeComponent, then GlobalComponent)
        void HandleEvent(std::shared_ptr<Event>& event);

        /// Cleans up all memory, GPU structures, etc...
        virtual void Cleanup();

        inline virtual ~NScene() { Cleanup(); }

        HSK_PROPERTY_ALL(Context)

      protected:
        const VkContext* mContext;
        /// @brief Buffer holding ownership of all nodes
        /// @remark Holds unique ptrs to be able to preserve pointers if the buffer is changed or moved. These pointers may be null-equivalent.
        std::vector<std::unique_ptr<NNode>> mNodeBuffer;

        /// @brief All nodes directly attached to the root
        std::vector<NNode*> mRootNodes;

        RootRegistry mGlobalRootRegistry;
    };
}  // namespace hsk