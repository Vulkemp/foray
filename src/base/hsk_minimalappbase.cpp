#include "hsk_minimalappbase.hpp"

#include <sdl2/SDL.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <exception>

#include "hsk_logger.hpp"

namespace hsk
{
    using clock_t = std::chrono::steady_clock;
    using timepoint_t = std::chrono::steady_clock::time_point;
    using timespan_t = std::chrono::duration<float>;

    int32_t MinimalAppBase::Run()
    {
        if (mState != EState::Uninitialized)
        {
            return -1;
        }

        try
        {
            this->State(EState::Preparing);
            this->BaseInitSdlSubsystem();
            this->BeforeInstanceCreate(mVkbInstanceBuilder);
            this->BaseInitVulkanInstance();
            this->Init();
        }
        catch (const std::exception &e)
        {
            spdlog::error("Exception thrown during initialization: {}", e.what());
            return -1;
        }

        try
        {
            clock_t clock;
            float deltaMillis = 0;
            this->State(EState::Running);
            timespan_t timePerTick(UpdateTiming.SecondsPerUpdate());
            timepoint_t lastTick = clock.now() - std::chrono::duration_cast<clock_t::duration>(timePerTick);

            timespan_t balance = timespan_t(0); // The balance variable is meant to smooth out the inconsistent sleep durations over time

            while (this->State() == EState::Running)
            {
                BasePollEvents(); // First, poll for new OS events

                timePerTick = timespan_t(UpdateTiming.SecondsPerUpdate()); // Recalculate time per tick, as it may have been changed

                timepoint_t now = clock.now();
                timespan_t delta = now - lastTick;

                if (delta + balance >= timePerTick)
                {
                    // sufficient time has past since last tick, so we update
                    lastTick = now;
                    balance += delta - timePerTick;

                    if (balance.count() > UpdateTiming.SecondsPerUpdate() * 5)
                    {
                        // We don't want to attempt smooth out more than 5 missed cycles
                        balance = timespan_t(0.f);
                    }
                    Render(delta.count());
                }
                else
                {
                    SDL_Delay(0);
                }
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Exception thrown during runtime: {}", e.what());
            return -1;
        }

        try
        {
            this->State(EState::Finalizing);
            Cleanup();
            BaseCleanupVkInstance();
            BaseCleanupSdlSubsystem();
        }
        catch (const std::exception &e)
        {
            spdlog::error("Exception thrown during deconstruct: {}", e.what());
            return -1;
        }

        this->State(EState::Uninitialized);
        return 0;
    }

    void MinimalAppBase::BaseInitSdlSubsystem(){}
    void MinimalAppBase::BaseInitVulkanInstance(){
        //mVkbInstanceBuilder.enable_extension("not available extension to produce error");
        auto instanceBuildRet = mVkbInstanceBuilder.build();
       
        if (!instanceBuildRet)
        {
            logger()->error("Create vkInst failed: {}", instanceBuildRet.error().message());
            return;
        }

        mVkbInstance = instanceBuildRet.value();
        mInstance = mVkbInstance.instance;
    }
    void MinimalAppBase::BasePollEvents(){}
    void MinimalAppBase::BaseCleanupVkInstance(){}
    void MinimalAppBase::BaseCleanupSdlSubsystem(){}
}