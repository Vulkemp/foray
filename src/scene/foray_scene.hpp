#pragma once
#include "../osi/foray_osi_declares.hpp"
#include "foray_callbackdispatcher.hpp"
#include "foray_node.hpp"
#include "foray_registry.hpp"
#include "foray_scenedrawing.hpp"
#include "foray_scene_declares.hpp"
#include "../as/foray_tlas.hpp"

namespace foray::scene {

    /// @brief Provides registries and methods as the anchor of a component based scene.
    /// @remark Manages and owns all nodes
    /// @remark Inherits Registry, and functions as the component registry for all global components
    /// @remark Inherits CallbackDispatcher for managing callbacks used by node components, and as a way for nodes to link back to the global instance
    class Scene : public Registry, public CallbackDispatcher
    {
      public:
        friend Node;

        explicit Scene(const core::VkContext* context);

        /// @brief Generates a new node and attaches it to the parent if it is set, root otherwise
        Node* MakeNode(Node* parent = nullptr);

        /// @brief Advance scene state by invoking all NodeComponent update callbacks, followed by GlobalComponent update callbacks
        void Update(const base::FrameUpdateInfo& updateInfo);
        /// @brief Draw the scene by first invoking all BeforeDraw callbacks (NodeComponent, then GlobalComponent), followed by Draw callbacks (NodeComponent, then GlobalComponent).
        void Draw(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout);
        /// @brief Invokes event callbacks (NodeComponent, then GlobalComponent)
        void HandleEvent(const Event* event);

        /// Cleans up all memory, GPU structures, etc...
        virtual void Destroy(bool reinitialize = false);

        inline virtual ~Scene() { Destroy(); }

        FORAY_PROPERTY_ALL(Context)
        FORAY_PROPERTY_ALL(NodeBuffer)
        FORAY_PROPERTY_ALL(RootNodes)

        template <typename TComponent>
        int32_t FindNodesWithComponent(std::vector<Node*>& outnodes);

      protected:
        const core::VkContext* mContext;
        /// @brief Buffer holding ownership of all nodes
        /// @remark Holds unique ptrs to be able to preserve pointers if the buffer is changed or moved. These pointers may be null-equivalent.
        std::vector<std::unique_ptr<Node>> mNodeBuffer;

        /// @brief All nodes directly attached to the root
        std::vector<Node*> mRootNodes;

        CallbackDispatcher mGlobalRootRegistry;

        void InitDefaultGlobals();
    };

    template <typename TComponent>
    int32_t Scene::FindNodesWithComponent(std::vector<Node*>& outnodes)
    {
        int32_t found = 0;
        for(Node* rootnode : mRootNodes)
        {
            if(rootnode->HasComponent<TComponent>())
            {
                found++;
                outnodes.push_back(rootnode);
            }
            found += rootnode->FindChildrenWithComponent<TComponent>(outnodes);
        }
        return found;
    }

}  // namespace foray