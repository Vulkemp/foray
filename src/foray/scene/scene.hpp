#pragma once
#include "../as/tlas.hpp"
#include "../osi/osi_declares.hpp"
#include "callbackdispatcher.hpp"
#include "node.hpp"
#include "registry.hpp"
#include "scene_declares.hpp"
#include "scenedrawing.hpp"
#include "../event.hpp"
#include "../stages/renderdomain.hpp"

namespace foray::scene {

    /// @brief Provides registries and methods as the anchor of a component based scene.
    /// @remark Manages and owns all nodes
    /// @remark Inherits Registry, and functions as the component registry for all global components
    /// @remark Inherits CallbackDispatcher for managing callbacks used by node components, and as a way for nodes to link back to the global instance
    class Scene : public Registry, public CallbackDispatcher
    {
      public:
        friend Node;

        explicit Scene(core::Context* context);

        /// @brief Generates a new node and attaches it to the parent if it is set, root otherwise
        Node* MakeNode(Node* parent = nullptr);

        /// @brief Advance scene state by invoking all NodeComponent update callbacks, followed by GlobalComponent update callbacks
        void Update(VkCommandBuffer cmdBuffer, const base::FrameRenderInfo& renderInfo, stages::RenderDomain* domain);
        /// @brief Draw the scene by invoking the Draw callbacks
        void Draw(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, VkCommandBuffer cmdBuffer);

        /// Cleans up all memory, GPU structures, etc...
        virtual void Destroy() override;

        inline virtual ~Scene() { Destroy(); }

        FORAY_PROPERTY_R(NodeBuffer)
        FORAY_PROPERTY_R(RootNodes)
        FORAY_PROPERTY_V(Context)

        template <typename TComponent>
        int32_t FindComponents(std::vector<TComponent*>& outcomponents);

        template <typename TComponent>
        int32_t FindNodesWithComponent(std::vector<Node*>& outnodes);

        /// @brief Adds a default camera to the scene (standard perspective + freecameracontroller) and selects it in the cameramanager
        void UseDefaultCamera(bool invertAll = false);
        /// @brief Rebuilds the Tlas. If your project requires a Tlas this must be called after altering the scene
        void UpdateTlasManager();        
        /// @brief Updates lights. If your project requires punctual lights, this must be called after altering the scene
        void UpdateLightManager();

      protected:
        core::Context* mContext;
        /// @brief Buffer holding ownership of all nodes
        /// @remark Holds unique ptrs to be able to preserve pointers if the buffer is changed or moved. These pointers may be null-equivalent.
        std::vector<std::unique_ptr<Node>> mNodeBuffer;

        /// @brief All nodes directly attached to the root
        std::vector<Node*> mRootNodes;

        event::Receiver<const osi::Event*> mOnOsEvent;

        void InitDefaultGlobals();

        void HandleEvent(const osi::Event* event);
    };

    template <typename TComponent>
    int32_t Scene::FindComponents(std::vector<TComponent*>& outnodes)
    {
        int32_t found = 0;
        for(Node* rootnode : mRootNodes)
        {
            found += rootnode->FindComponentsRecursive<TComponent>(outnodes);
        }
        return found;
    }

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
}  // namespace foray::scene