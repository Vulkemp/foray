#include "foray_renderloop.hpp"
#include "../foray_logger.hpp"
#include <chrono>
#include <nameof/nameof.hpp>
#include <sdl2/SDL.h>

namespace foray::base {
    using clock_t     = std::chrono::steady_clock;
    using timepoint_t = std::chrono::steady_clock::time_point;
    using timespan_t  = std::chrono::duration<float>;

    int32_t RenderLoop::Run()
    {
        if(mState != ELifetimeState::PreInit)
        {
            return -1;
        }

        bool hasRenderFunc = !!mRenderFunc;

        if(!hasRenderFunc)
        {
            logger()->warn("[RenderLoop::Run] called with the following recommended functions not configured:{}", (hasRenderFunc ? "" : " RenderFunc"));
        }

#ifdef FORAY_CATCH_EXCEPTIONS
        try
        {
#endif
            AdvanceState();  // ELifetimeState::Initializing
            if(!!mInitFunc)
            {
                mInitFunc();
            }

            AdvanceState();  // ELifetimeState::Running

            clock_t     clock;
            float       deltaMillis = 0;
            timespan_t  timePerTick(mFrameTiming.GetSecondsPerFrame());
            timepoint_t lastTick = clock.now() - std::chrono::duration_cast<clock_t::duration>(timePerTick);
            timepoint_t start    = clock.now();

            timespan_t balance = timespan_t(0);  // The balance variable is meant to smooth out the inconsistent sleep durations over time

            while(mState == ELifetimeState::Running)
            {
                if(!!mPollEventsFunc)
                {
                    mPollEventsFunc();
                }

                timePerTick = timespan_t(mFrameTiming.GetSecondsPerFrame());  // Recalculate time per tick, as it may have been changed

                timepoint_t now        = clock.now();
                timespan_t  delta      = now - lastTick;
                timespan_t  sinceStart = now - start;

                bool canRender = delta + balance >= timePerTick;
                if(canRender && !!mRenderReadyFunc)
                {
                    canRender = mRenderReadyFunc();
                }

                while(mFrameTimes.size() > 0 && mFrameTimes.front().Timestamp + mMaxFrameTimeAge < sinceStart.count())
                {
                    mFrameTimes.erase(mFrameTimes.begin());
                }

                if(canRender)
                {
                    // sufficient time has passed since last tick, so we update
                    lastTick = now;
                    balance += delta - timePerTick;

                    if(balance > timePerTick * 5)
                    {
                        // We don't want to attempt smooth out more than 5 missed cycles
                        balance = timespan_t(0.f);
                    }
                    if(mState == ELifetimeState::Running)
                    {
                        FrameTime frameTime{.Delta = delta.count(), .Timestamp = sinceStart.count()};
                        mFrameTimes.emplace_back(frameTime);

                        if(!!mRenderFunc)
                        {
                            mRenderFunc(frameTime.Delta);
                            mRenderedFrameCount++;
                        }
                    }
                }
                else
                {
                    SDL_Delay(0);
                }
            }

            AdvanceState();  // ELifetimeState::Destroying

            if(!!mDestroyFunc)
            {
                mDestroyFunc();
            }

#ifdef FORAY_CATCH_EXCEPTIONS
        }

        catch(const std::exception& e)
        {
            logger()->error("Exception thrown during {}: {}", NAMEOF_ENUM(mState) e.what());
            return -1;
        }
#endif

        // This will not call any callbacks, as it might have already been finalized
        mState = ELifetimeState::Finalized;
        return mRunResult;
    }

    RenderLoop& RenderLoop::SetInitFunc(InitFunctionPointer func)
    {
        mInitFunc = func;
        return *this;
    }
    RenderLoop& RenderLoop::SetRenderFunc(RenderFunctionPointer func)
    {
        mRenderFunc = func;
        return *this;
    }
    RenderLoop& RenderLoop::SetRenderReadyFunc(RenderReadyFunctionPointer func)
    {
        mRenderReadyFunc = func;
        return *this;
    }
    RenderLoop& RenderLoop::SetDestroyFunc(DestroyFunctionPointer func)
    {
        mDestroyFunc = func;
        return *this;
    }
    RenderLoop& RenderLoop::SetPollEventsFunc(PollEventsFunctionPointer func)
    {
        mPollEventsFunc = func;
        return *this;
    }
    RenderLoop& RenderLoop::SetOnStateChangedFunc(OnStateChangedFunctionPointer func)
    {
        mOnStateChangedFunc = func;
        return *this;
    }

    void RenderLoop::RequestStop(int32_t runResult)
    {
        if(mState == ELifetimeState::Running)
        {
            AdvanceState();  // ELifetimeState::StopRequested
        }
        mRunResult = runResult;
    }

    RenderLoop::FrameTimeAnalysis RenderLoop::AnalyseFrameTimes() const
    {
        FrameTimeAnalysis result;
        fp64_t            frameTimeSum = 0.0;
        for(const FrameTime& frametime : mFrameTimes)
        {
            frameTimeSum += (fp64_t)frametime.Delta;
            result.MinFrameTime = std::min(result.MinFrameTime, frametime.Delta);
            result.MaxFrameTime = std::max(result.MaxFrameTime, frametime.Delta);
        }
        result.TotalTime    = frameTimeSum;
        result.Count        = (uint32_t)mFrameTimes.size();
        result.AvgFrameTime = (fp32_t)(frameTimeSum / mFrameTimes.size());
        return result;
    }

    void RenderLoop::GetFrameTimes(std::vector<fp32_t>& outFrameTimes) const
    {
        for(const FrameTime& frametime : mFrameTimes)
        {
            outFrameTimes.push_back(frametime.Delta);
        }
    }

    void RenderLoop::AdvanceState()
    {
        ELifetimeState old  = mState;
        ELifetimeState next = (ELifetimeState)(((int32_t)mState) + 1);
        if(!!mOnStateChangedFunc)
        {
            mOnStateChangedFunc(old, next);
        }
        mState = next;
    }
}  // namespace foray::base
