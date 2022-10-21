#pragma once
#include "../foray_basics.hpp"
#include <functional>
#include <limits>
#include <list>
#include <vector>

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

    void PrintStateChange(ELifetimeState oldState, ELifetimeState newState);

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

    /// @brief Manages a single threaded, automatically balancing application lifetime
    class RenderLoop
    {
      public:
        struct RenderInfo
        {
            fp32_t   Delta           = 0.f;
            fp32_t   TargetDelta     = 0.f;
            uint64_t LoopFrameNumber = 0;
            fp64_t   SinceStart      = 0.0;
        };

        /// @brief Function pointer for application initialization
        using InitFunctionPointer = std::function<void()>;
        /// @brief Function pointer for a single frame render action. Param#0 : Delta time in seconds
        using RenderFunctionPointer = std::function<void(RenderInfo&)>;
        /// @brief Function pointer for the RenderLoop to check if application is ready to render next frame. Return true if ready.
        using RenderReadyFunctionPointer = std::function<bool()>;
        /// @brief Function pointer for application finalization
        using DestroyFunctionPointer = std::function<void()>;
        /// @brief Function pointer for system event polling and handling
        using PollEventsFunctionPointer = std::function<void()>;
        /// @brief Function pointer for a callback when the renderloops state changes. Param#0: Old state Param#1: Next state
        using OnStateChangedFunctionPointer = std::function<void(ELifetimeState, ELifetimeState)>;

        inline RenderLoop() = default;
        /// @brief Constructor
        /// @param initFunc Function called once to initialize the application
        /// @param renderFunc Function called once per frame to render the application
        /// @param renderReadyFunc Function called 1...x times per frame to check if the application is ready for the next frame
        /// @param destroyFunc Function called once to destroy application resources
        /// @param pollEventsFunc Function called 1...x times per frame to poll and handle system events
        /// @param onStateChangedFunc Function called everytime the renderloop state changes
        inline RenderLoop(InitFunctionPointer           initFunc,
                          RenderFunctionPointer         renderFunc,
                          RenderReadyFunctionPointer    renderReadyFunc,
                          DestroyFunctionPointer        destroyFunc,
                          PollEventsFunctionPointer     pollEventsFunc,
                          OnStateChangedFunctionPointer onStateChangedFunc)
            : mInitFunc{initFunc}
            , mRenderFunc{renderFunc}
            , mRenderReadyFunc{renderReadyFunc}
            , mDestroyFunc{destroyFunc}
            , mPollEventsFunc{pollEventsFunc}
            , mOnStateChangedFunc{onStateChangedFunc} {};

        /// @brief Runs the application through its full lifetime, invoking all function pointers which have been set
        /// @return The runResult value from RequestStop, or -1 on catched exception
        int32_t Run();
        /// @brief Request the finalization of the application as soon as possible
        /// @param runResult The result the encompassing Run() call should return
        void RequestStop(int32_t runResult = 0);
        /// @brief Returns true, if the internal state is ELifetimeState::Running; false otherwise
        bool IsRunning() const;

        /// @brief Analysis
        struct FrameTimeAnalysis
        {
            /// @brief Total time in seconds of the analyzed frametimes
            fp32_t TotalTime = 0.f;
            /// @brief Count of frametimes analyzed
            uint32_t Count = 0;
            /// @brief Minimum frame time in seconds recorded
            fp32_t MinFrameTime = std::numeric_limits<fp32_t>::infinity();
            /// @brief Maximum frame time in seconds recorded
            fp32_t MaxFrameTime = 0.f;
            /// @brief Average frame time in seconds recorded
            fp32_t AvgFrameTime = 0.f;
        };

        /// @brief Analyse all stored frame times. Note: MaxFrameTimeAge member determines maximum possible frame time age
        FrameTimeAnalysis AnalyseFrameTimes() const;
        /// @brief Get all stored frame times (in seconds) by pushing them on the vector
        void GetFrameTimes(std::vector<fp32_t>& outFrameTimes) const;

        RenderLoop& SetInitFunc(InitFunctionPointer func);
        RenderLoop& SetRenderFunc(RenderFunctionPointer func);
        RenderLoop& SetRenderReadyFunc(RenderReadyFunctionPointer func);
        RenderLoop& SetDestroyFunc(DestroyFunctionPointer func);
        RenderLoop& SetPollEventsFunc(PollEventsFunctionPointer func);
        RenderLoop& SetOnStateChangedFunc(OnStateChangedFunctionPointer func);

        FORAY_PROPERTY_CGET(State)

        FORAY_PROPERTY_GET(FrameTiming)
        FORAY_PROPERTY_ALL(MaxFrameTimeAge)

      protected:
        void AdvanceState();

        InitFunctionPointer           mInitFunc           = nullptr;
        RenderFunctionPointer         mRenderFunc         = nullptr;
        RenderReadyFunctionPointer    mRenderReadyFunc    = nullptr;
        DestroyFunctionPointer        mDestroyFunc        = nullptr;
        PollEventsFunctionPointer     mPollEventsFunc     = nullptr;
        OnStateChangedFunctionPointer mOnStateChangedFunc = nullptr;

        ELifetimeState mState     = ELifetimeState::PreInit;
        int32_t        mRunResult = 0;

        AppFrameTiming mFrameTiming;
        uint64_t       mRenderedFrameCount = 0;


        struct FrameTime
        {
            fp32_t Delta     = 0.f;
            fp64_t Timestamp = 0;
        };

        std::list<FrameTime> mFrameTimes;
        fp32_t               mMaxFrameTimeAge = 1.f;
    };
}  // namespace foray::base
