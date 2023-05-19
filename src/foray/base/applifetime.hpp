#pragma once
#include "../basics.hpp"
#include "base_declares.hpp"
#include <functional>

namespace foray::base {


    /// @brief The lifetime states an application progresses through chronologically
    enum class ELifetimeState
    {
        PreInit,
        Initializing,
        Running,
        StopRequested,
        Destroying,
        Finalized
    };

    /// @brief Controls an apps render scheduling timing
    class AppFrameTiming
    {
      public:
        /// @brief Set the target render calls per second
        inline void SetFramesPerSecond(float value) { mSecondsPerFrame = 1.f / value; }
        /// @brief Set the target time per frame
        inline void SetSecondsPerFrame(float value) { mSecondsPerFrame = value; }
        /// @brief Gets target frames per second
        inline float GetFramesPerSecond() const { return 1.f / mSecondsPerFrame; }
        /// @brief Gets target seconds per frame
        inline float GetSecondsPerFrame() const { return mSecondsPerFrame; }

        /// @brief Sets the target Fps to 60 (0.166667 seconds per frame)
        inline void Set60Fps() { mSecondsPerFrame = 1.f / 60.f; }
        /// @brief Disables the Fps limit
        inline void DisableFpsLimit() { mSecondsPerFrame = 0; }

      protected:
        fp32_t mSecondsPerFrame = 1.f / 60.f;
    };

    /// @brief Info describing a loop iteration
    struct LoopInfo
    {
        /// @brief Delta time in seconds of current loop iteration since previous
        fp32_t   Delta           = 0.f;
        /// @brief Targeted delta time
        fp32_t   TargetDelta     = 0.f;
        /// @brief Iteration index
        uint64_t LoopFrameNumber = 0;
        /// @brief Total time since application launched
        fp64_t   SinceStart      = 0.0;
    };

    class IApplication
    {
      public:
        /// @brief Initialization, called once at start
        virtual void IApplicationInit() {}
        /// @brief Called once per loop iteration
        virtual void IApplicationLoop(LoopInfo&) {}
        /// @brief Called before attempting a loop iteration
        /// @return True indicates the application is ready for next iteration, false otherwise
        virtual bool IApplicationLoopReady() { return true; }
        /// @brief Called in between processing of loop iterations to process events
        virtual void IApplicationProcessEvents() {}
        /// @brief Virtual destructor
        virtual ~IApplication() = default;
    };

    class FuncPtrApplication
    {
      public:
        /// @brief Function pointer for application initialization
        using InitFunctionPointer = std::function<void()>;
        /// @brief Function pointer for a single frame render action. Param#0 : Delta time in seconds
        using RenderFunctionPointer = std::function<void(LoopInfo&)>;
        /// @brief Function pointer for the RenderLoop to check if application is ready to render next frame. Return true if ready.
        using RenderReadyFunctionPointer = std::function<bool()>;
        /// @brief Function pointer for application finalization
        using DestroyFunctionPointer = std::function<void()>;
        /// @brief Function pointer for system event polling and handling
        using PollEventsFunctionPointer = std::function<void()>;
        /// @brief Constructor
        /// @param initFunc Function called once to initialize the application
        /// @param renderFunc Function called once per frame to render the application
        /// @param renderReadyFunc Function called 1...x times per frame to check if the application is ready for the next frame
        /// @param destroyFunc Function called once to destroy application resources
        /// @param pollEventsFunc Function called 1...x times per frame to poll and handle system events
        /// @param onStateChangedFunc Function called everytime the renderloop state changes
        FuncPtrApplication(InitFunctionPointer        initFunc,
                           RenderFunctionPointer      renderFunc,
                           RenderReadyFunctionPointer renderReadyFunc,
                           DestroyFunctionPointer     destroyFunc,
                           PollEventsFunctionPointer  pollEventsFunc)
            : mInitFunc{initFunc}, mRenderFunc{renderFunc}, mRenderReadyFunc{renderReadyFunc}, mDestroyFunc{destroyFunc}, mPollEventsFunc{pollEventsFunc}
        {
        }

        virtual void IApplicationInit()
        {
            if(!!mInitFunc)
            {
                mInitFunc();
            }
        }
        virtual void IApplicationLoop(LoopInfo& loopInfo)
        {
            if(!!mRenderFunc)
            {
                mRenderFunc(loopInfo);
            }
        }
        virtual bool IApplicationLoopReady()
        {
            if(!!mRenderReadyFunc)
            {
                return mRenderReadyFunc();
            }
            return true;
        }
        virtual void IApplicationProcessEvents()
        {
            if(!!mPollEventsFunc)
            {
                mPollEventsFunc();
            }
        }
        virtual ~FuncPtrApplication()
        {
            if(!!mDestroyFunc)
            {
                mDestroyFunc();
            }
        }

      protected:
        InitFunctionPointer        mInitFunc        = nullptr;
        RenderFunctionPointer      mRenderFunc      = nullptr;
        RenderReadyFunctionPointer mRenderReadyFunc = nullptr;
        DestroyFunctionPointer     mDestroyFunc     = nullptr;
        PollEventsFunctionPointer  mPollEventsFunc  = nullptr;
    };

}  // namespace foray::base
