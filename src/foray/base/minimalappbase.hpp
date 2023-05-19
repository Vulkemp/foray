#pragma once
#include "../core/context.hpp"
#include "../logger.hpp"
#include "../osi/osmanager.hpp"
#include "../mem.hpp"
#include "applifetime.hpp"
#include "vulkaninstance.hpp"

namespace foray::base {
    /// @brief Application base providing bare minimum of functionality (app lifetime, event handling, vulkan instance management)
    class MinimalAppBase : public IApplication
    {
      public:
        MinimalAppBase(AppLoopBase* apploop);
        virtual ~MinimalAppBase();

        FORAY_GETTER_MR(OsManager)
        FORAY_GETTER_MR(Instance)

      protected:
        /// @brief Inits the applications basic vulkan objects
        virtual void IApplicationInit();
        /// @brief Polls and distributes events from the SDL subsystem
        virtual void IApplicationProcessEvents();

        /// @brief Override this method to alter vulkan instance creation parameters via the instance builder
        inline virtual void ApiBeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) {}
        /// @brief Override this method to react to events
        inline virtual void ApiOnOsEvent(const osi::Event* event) {}

        /// @brief [Internal]
        inline virtual void OnOsEvent(const osi::Event* event);

        AppLoopBase*     mAppLoop;
        osi::OsManager mOsManager;
        Heap<VulkanInstance> mInstance;
        core::Context  mContext;

        event::Receiver<const osi::Event*> mOsEventReceiver;
    };
}  // namespace foray::base