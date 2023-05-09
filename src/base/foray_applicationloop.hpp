#pragma once
#include "../foray_mem.hpp"
#include "foray_applifetime.hpp"
#include <limits>
#include <list>

namespace foray::base {
    /// @brief Manages a single threaded, automatically balancing application lifetime
    class AppLoopBase
    {
      public:
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

        FORAY_GETTER_V(State)

        FORAY_GETTER_MR(FrameTiming)
        FORAY_PROPERTY_V(MaxFrameTimeAge)

        static fp32_t GetTime();
        static void   ThreadSleep(fp32_t time);

      protected:
        void PruneFrameTimes();

        void AdvanceState();
        void PrintStateChange(ELifetimeState oldState, ELifetimeState newState);

        void LogException(const std::exception& exc);

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

    template <typename TApp>
    class ApplicationLoop : public AppLoopBase
    {
      public:
        inline int32_t Run()
        {
            if(mState != ELifetimeState::PreInit)
            {
                return -1;
            }

            Local<TApp> app;

            app.New(this);

#ifdef FORAY_CATCH_EXCEPTIONS
            try
            {
#endif

                AdvanceState();  // ELifetimeState::Initializing

                app->IApplicationInit();

                AdvanceState();  // ELifetimeState::Running

                fp32_t timePerTick = mFrameTiming.GetSecondsPerFrame();
                fp32_t lastTick    = GetTime() - timePerTick;

                fp32_t balance = 0;

                while(mState == ELifetimeState::Running)
                {
                    app->IApplicationProcessEvents();

                    timePerTick = mFrameTiming.GetSecondsPerFrame();  // Recalculate time per tick, as it may have been changed

                    fp32_t now   = GetTime();
                    fp32_t delta = now - lastTick;

                    bool canRender = delta + balance >= timePerTick;
                    canRender      = canRender && app->IApplicationLoopReady();

                    if(canRender)
                    {
                        // sufficient time has passed since last tick, so we update
                        lastTick = now;
                        balance += delta - timePerTick;

                        if(balance > timePerTick * 5)
                        {
                            // We don't want to attempt smooth out more than 5 missed cycles
                            balance = 0.f;
                        }
                        if(mState == ELifetimeState::Running)
                        {
                            FrameTime frameTime{.Delta = delta, .Timestamp = now};
                            mFrameTimes.emplace_back(frameTime);
                            PruneFrameTimes();

                            LoopInfo loopInfo{.Delta = delta, .TargetDelta = mFrameTiming.GetSecondsPerFrame(), .LoopFrameNumber = mRenderedFrameCount, .SinceStart = now};

                            app->IApplicationLoop(loopInfo);
                            mRenderedFrameCount++;
                        }
                    }
                    else
                    {
                        ThreadSleep(0);
                    }
                }

                AdvanceState();  // ELifetimeState::Destroying

                app.Delete();

#ifdef FORAY_CATCH_EXCEPTIONS
            }

            catch(const std::exception& e)
            {
                LogException(e);
                return -1;
            }
#endif

            // This will not call any callbacks, as it might have already been finalized
            mState = ELifetimeState::Finalized;
            return mRunResult;
        }
    };
}  // namespace foray::base
