#pragma once
#include "../osi/foray_osi.hpp"
#include "../foray_logger.hpp"
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
        float           mFps{0};
        float           mMsPerFrame{0};
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
        inline virtual bool CanRenderNextFrame() { return false; }
        /// @brief Override this method to render your application
        inline virtual void Render(float delta) { }
         /// @brief Override to update logic. Delta describes time passed since last update.
         inline virtual void Update(float delta) {}
        /// @brief Override this method to react to events
        inline virtual void OnEvent(const Event* event) {}
        /// @brief Override this method to cleanup your application
        inline virtual void Destroy() {}
#pragma endregion

#pragma region Vulkan
        vkb::InstanceBuilder mVkbInstanceBuilder{};
        vkb::Instance        mInstanceVkb{};
        VkInstance           mInstance{};
        bool                 mDebugEnabled{true};
#pragma endregion
    };
}  // namespace foray