#pragma once
#include "../core/foray_context.hpp"
#include "../foray_logger.hpp"
#include "../osi/foray_osmanager.hpp"
#include "foray_renderloop.hpp"
#include "foray_vulkaninstance.hpp"

namespace foray::base {
    /// @brief Application base providing bare minimum of functionality (app lifetime, event handling, vulkan instance management)
    class MinimalAppBase
    {
      public:
        MinimalAppBase(bool printStateChanges = true);
        virtual ~MinimalAppBase() = default;

        /// @brief Runs through the entire lifetime of the app
        int32_t Run();

        FORAY_PROPERTY_ALLGET(RenderLoop)
        FORAY_PROPERTY_ALLGET(OsManager)
        FORAY_PROPERTY_ALLGET(Instance)

      protected:
        /// @brief Inits the applications basic vulkan objects
        virtual void Init();
        /// @brief Polls and distributes events from the SDL subsystem
        virtual void PollEvents();
        /// @brief Destroys the vulkan instance
        virtual void Destroy();

        /// @brief Override this method to alter vulkan instance creation parameters via the instance builder
        inline virtual void ApiBeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) {}
        /// @brief Override this method to init your application
        inline virtual bool ApiCanRenderNextFrame() { return true; }
        /// @brief Override this method to render your application
        inline virtual void ApiRender(RenderLoop::RenderInfo& renderInfo) {}
        /// @brief Override this method to react to events
        inline virtual void ApiOnEvent(const osi::Event* event) {}
        /// @brief Override this method to cleanup your application
        inline virtual void ApiDestroy() {}

        RenderLoop     mRenderLoop;
        osi::OsManager mOsManager;
        VulkanInstance mInstance;
        core::Context  mContext;
    };
}  // namespace foray::base