#pragma once
#include "../base/framerenderinfo.hpp"
#include "../core/core_declares.hpp"
#include "../basics.hpp"
#include "../event.hpp"
#include "../osi/osi_declares.hpp"
#include "scene_declares.hpp"
#include "scenedrawing.hpp"

namespace foray::scene {

    /// @brief Base class for all types manageable by registry
    class Component : public NoMoveDefaults
    {
      public:
        friend Registry;

        /// @brief Base class for implementing the update callback
        class UpdateCallback : public event::PriorityReceiver<TUpdateMessage>
        {
          public:
            inline UpdateCallback(int32_t priority)
                : event::PriorityReceiver<TUpdateMessage>(
                    nullptr, [this](TUpdateMessage msg) { this->Update(msg); }, priority)
            {
            }

            virtual ~UpdateCallback() = default;

            /// @brief Invoked first each frame. Use for changes to the node hierarchy and transforms
            inline virtual void Update(TUpdateMessage updateInfo) = 0;

            static const int32_t ORDER_TRANSFORM    = 100;
            static const int32_t ORDER_DEVICEUPLOAD = 200;
        };

        /// @brief Base class for implementing the draw callback
        class DrawCallback : public event::Receiver<TDrawMessage>
        {
          public:
            inline DrawCallback() : event::Receiver<TDrawMessage>(
                    nullptr, [this](TDrawMessage msg) { this->Draw(msg); }){}

            virtual ~DrawCallback() = default;

            /// @brief Invoked last each frame. Use to submit draw calls (and related)
            inline virtual void Draw(TDrawMessage drawInfo) = 0;
        };

        /// @brief Base class for implementing the onevent callback
        class OnEventCallback : public event::Receiver<TOsEventMessage>
        {
          public:
            inline OnEventCallback() : event::Receiver<TOsEventMessage>(
                    nullptr, [this](TOsEventMessage msg) { this->OnOsEvent(msg); }){}

            virtual ~OnEventCallback() = default;

            /// @brief Invoked with every event received by the application
            inline virtual void OnOsEvent(TOsEventMessage event) = 0;
        };

        /// @brief Destructor. Provide virtual constructors in inheriting classes, to make sure they get finalized correctly.
        inline virtual ~Component() = default;

        FORAY_GETTER_V(Registry)
        FORAY_PROPERTY_R(Name)

        virtual Scene*         GetScene()   = 0;
        virtual Registry*      GetGlobals() = 0;
        virtual core::Context* GetContext() = 0;

      protected:
        Registry*   mRegistry = nullptr;
        std::string mName     = "";
    };

    class NodeComponent : public Component
    {
      public:
        /// @brief Node this component is attached to. Casts mRegistry to Node.
        Node* GetNode();
        /// @brief Scene this component is a part of. Casts mRegistry->mCallbackDispatcher to Scene
        virtual Scene* GetScene() override;
        /// @brief Global component registry. Casts mRegistry->mCallbackDispatcher to Scene and returns Scene->GetGlobals()
        virtual Registry* GetGlobals() override;
        /// @brief Vulkan Context. Casts mRegistry->mCallbackDispatcher to Scene and returns Scene->GetContext()
        virtual core::Context* GetContext() override;
    };

    class GlobalComponent : public Component
    {
      public:
        /// @brief Scene this component is a part of. Casts mRegistry to Scene
        virtual Scene* GetScene() override;
        /// @brief Global component registry. Returns mRegistry
        virtual Registry* GetGlobals() override;
        /// @brief Vulkan Context. Casts mRegistry to Scene and returns Scene->GetContext()
        virtual core::Context* GetContext() override;
    };
}  // namespace foray::scene