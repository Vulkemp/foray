#pragma once
#include "../base/foray_framerenderinfo.hpp"
#include "../core/foray_core_declares.hpp"
#include "../foray_basics.hpp"
#include "../osi/foray_osi_declares.hpp"
#include "foray_scene_declares.hpp"
#include "foray_scenedrawing.hpp"

namespace foray::scene {

    /// @brief Base class for all types manageable by registry
    class Component : public NoMoveDefaults, public Polymorphic
    {
      public:
        friend Registry;

        /// @brief Base class for implementing the update callback
        class UpdateCallback : public Polymorphic
        {
          public:
            static const bool ORDERED_EXECUTION = true;
            using TArg                          = SceneUpdateInfo&;

            /// @brief Invoked first each frame. Use for changes to the node hierarchy and transforms
            inline virtual void Update(TArg updateInfo) = 0;
            inline void         Invoke(TArg updateInfo) { Update(updateInfo); }

            static const int32_t ORDER_TRANSFORM    = 100;
            static const int32_t ORDER_DEVICEUPLOAD = 200;

            virtual inline int32_t GetOrder() const { return 0; }
        };

        /// @brief Base class for implementing the draw callback
        class DrawCallback : public Polymorphic
        {
          public:
            static const bool ORDERED_EXECUTION = false;
            using TArg                          = SceneDrawInfo&;
            /// @brief Invoked last each frame. Use to submit draw calls (and related)
            inline virtual void Draw(TArg drawInfo) = 0;
            inline void         Invoke(TArg drawInfo) { Draw(drawInfo); }
        };

        /// @brief Base class for implementing the onevent callback
        class OnEventCallback : public Polymorphic
        {
          public:
            static const bool ORDERED_EXECUTION = false;
            using TArg                          = const Event*;
            /// @brief Invoked with every event received by the application
            inline virtual void OnEvent(TArg event) = 0;
            inline void         Invoke(TArg event) { OnEvent(event); }
        };

        class OnResizedCallback : public Polymorphic
        {
          public:
            static const bool ORDERED_EXECUTION = false;
            using TArg                          = VkExtent2D;
            /// @brief Invoked when the primary render resolution changes
            inline virtual void OnResized(TArg extent) = 0;
            inline void         Invoke(TArg event) { OnResized(event); }
        };

        /// @brief Destructor. Provide virtual constructors in inheriting classes, to make sure they get finalized correctly.
        inline virtual ~Component() {}

        FORAY_PROPERTY_CGET(Registry)
        FORAY_PROPERTY_GET(Registry)

        virtual Scene*                 GetScene()   = 0;
        virtual Registry*              GetGlobals() = 0;
        virtual core::Context* GetContext() = 0;

      protected:
        Registry* mRegistry = nullptr;
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