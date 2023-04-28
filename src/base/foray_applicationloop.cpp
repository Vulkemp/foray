#include "foray_applicationloop.hpp"
#include "../foray_logger.hpp"
#include "SDL.h"

namespace foray::base {
    void AppLoopBase::PrintStateChange(ELifetimeState oldState, ELifetimeState newState)
    {
        logger()->info("Lifetime State: {} => {}", NAMEOF_ENUM(oldState), NAMEOF_ENUM(newState));
    }

    void AppLoopBase::RequestStop(int32_t runResult)
    {
        if(mState == ELifetimeState::Running)
        {
            AdvanceState();  // ELifetimeState::StopRequested
        }
        mRunResult = runResult;
    }

    bool AppLoopBase::IsRunning() const
    {
        return mState == ELifetimeState::Running;
    }

    AppLoopBase::FrameTimeAnalysis AppLoopBase::AnalyseFrameTimes() const
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

    void AppLoopBase::GetFrameTimes(std::vector<fp32_t>& outFrameTimes) const
    {
        for(const FrameTime& frametime : mFrameTimes)
        {
            outFrameTimes.push_back(frametime.Delta);
        }
    }

    void AppLoopBase::PruneFrameTimes()
    {
        fp32_t now = GetTime();
        while(mFrameTimes.size() > 0 && mFrameTimes.front().Timestamp + mMaxFrameTimeAge < now)
        {
            mFrameTimes.erase(mFrameTimes.begin());
        }
    }

    void AppLoopBase::AdvanceState()
    {
        ELifetimeState old = mState;
        mState = (ELifetimeState)(((int32_t)mState) + 1);
        PrintStateChange(old, mState);
    }

    void AppLoopBase::LogException(const std::exception& exc)
    {
        logger()->error("Fatal error at {}: {}", NAMEOF_ENUM(mState), exc.what());
    }

    fp32_t AppLoopBase::GetTime()
    {
        static uint64_t START         = SDL_GetPerformanceCounter();
        fp64_t          perfCntrValue = (fp64_t)(SDL_GetPerformanceCounter() - START);
        fp64_t          perfCntrFreq  = (fp64_t)SDL_GetPerformanceFrequency();
        return (fp32_t)(perfCntrValue / perfCntrFreq);
    }
    void AppLoopBase::ThreadSleep(fp32_t time)
    {
        return SDL_Delay((uint32_t)(time * 1000.f));
    }

}  // namespace foray::base