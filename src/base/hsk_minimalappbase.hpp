#pragma once
#include "../osi/hsk_osi.hpp"
#include "hsk_logger.hpp"
#include <stdint.h>
#include <vkbootstrap/VkBootstrap.h>

namespace hsk {
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
        MinimalAppBase()          = default;
        virtual ~MinimalAppBase() = default;

#pragma region Lifetime

        /// @brief Enum representing application lifetime state
        enum class EState
        {
            /// @brief Application is not initialized
            Uninitialized,
            /// @brief Application is initializing
            Preparing,
            /// @brief Application is running
            Running,
            /// @brief Application is requested to stop
            StopRequested,
            /// @brief Application is cleaning up
            Finalizing
        };

        inline EState State() const { return mState; }

        /// @brief Runs through the entire lifetime of the app
        int32_t Run();

      protected:
        AppUpdateTiming mUpdateTiming;
        OsManager       mOsManager;
        EState          mState = EState::Uninitialized;
        inline void     State(EState state)
        {
            PrintStateChange(mState, state);
            mState = state;
        }

        void PrintStateChange(EState oldState, EState newState);

        /// @brief Inits the SDL subsystem
        virtual void BaseInitSdlSubsystem();
        /// @brief Inits the applications basic vulkan objects
        virtual void BaseInit();
        /// @brief Polls and distributes events from the SDL subsystem
        virtual void BasePollEvents();
        /// @brief Destroys the vulkan instance
        virtual void BaseCleanupVulkan();
        /// @brief Destroys the SDL subsystem
        virtual void BaseCleanupSdlSubsystem();

        /// @brief Override this method to alter vulkan instance creation parameters via the instance builder
        inline virtual void BeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) {}
        /// @brief Override this method to init your application
        inline virtual void Init() {}
        /// @brief Override this method to render your application
        inline virtual void Render(float delta) {}
        /// @brief Override this method to react to events
        inline virtual void OnEvent(std::shared_ptr<Event> event) {}
        /// @brief Override this method to cleanup your application
        inline virtual void Cleanup() {}
#pragma endregion

#pragma region Vulkan
        vkb::InstanceBuilder mVkbInstanceBuilder{};
        vkb::Instance        mInstanceVkb{};
        VkInstance           mInstance{};
        bool                 mEnableDefaultValidationLayers{true};
#pragma endregion
    };
}  // namespace hsk