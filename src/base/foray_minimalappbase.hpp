#pragma once
#include "../foray_logger.hpp"
#include "../osi/foray_osi.hpp"
#include "foray_renderloop.hpp"
#include "foray_vulkaninstance.hpp"
#include <stdint.h>
#include <vkbootstrap/VkBootstrap.h>

namespace foray::base {
    class AppUpdateTiming
    {
      public:
        inline void  UpdatesPerSecond(float value) { mSecondsPerUpdate = 1.f / value; }
        inline void  SecondsPerUpdate(float value) { mSecondsPerUpdate = value; }
        inline float UpdatesPerSecond() const { return 1.f / mSecondsPerUpdate; }
        inline float SecondsPerUpdate() const { return mSecondsPerUpdate; }

        inline void Set60Fps() { mSecondsPerUpdate = 1.f / 60.f; }
        inline void DisableFpsLock() { mSecondsPerUpdate = 0; }

      protected:
        float mSecondsPerUpdate = 1.f / 60.f;
    };

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
        inline virtual void ApiRender(float delta) {}
        /// @brief Override this method to react to events
        inline virtual void ApiOnEvent(const Event* event) {}
        /// @brief Override this method to cleanup your application
        inline virtual void ApiDestroy() {}

        RenderLoop     mRenderLoop;
        OsManager      mOsManager;
        VulkanInstance mInstance;
    };
}  // namespace foray::base