#include "hsk_minimalappbase.hpp"

#include "../osi/hsk_event.hpp"
#include "../osi/hsk_window.hpp"
#include <chrono>
#include <exception>
#include <nameof/nameof.hpp>
#include <sdl2/SDL.h>
#include <spdlog/spdlog.h>

namespace hsk {
    using clock_t     = std::chrono::steady_clock;
    using timepoint_t = std::chrono::steady_clock::time_point;
    using timespan_t  = std::chrono::duration<float>;

    int32_t MinimalAppBase::Run()
    {
        if(mState != EState::Uninitialized)
        {
            return -1;
        }

        try
        {
            this->State(EState::Preparing);
            this->BaseInitSdlSubsystem();
            this->BeforeInstanceCreate(mVkbInstanceBuilder);
            this->BaseInit();
            this->Init();
        }
        catch(const std::exception& e)
        {
            logger()->error("Exception thrown during initialization: {}", e.what());
            return -1;
        }

        try
        {
            clock_t clock;
            float   deltaMillis = 0;
            this->State(EState::Running);
            timespan_t  timePerTick(mUpdateTiming.SecondsPerUpdate());
            timepoint_t lastTick = clock.now() - std::chrono::duration_cast<clock_t::duration>(timePerTick);

            timespan_t balance = timespan_t(0);  // The balance variable is meant to smooth out the inconsistent sleep durations over time

            while(this->State() == EState::Running)
            {
                BasePollEvents();  // First, poll for new OS events

                timePerTick = timespan_t(mUpdateTiming.SecondsPerUpdate());  // Recalculate time per tick, as it may have been changed

                timepoint_t now   = clock.now();
                timespan_t  delta = now - lastTick;

                if(delta + balance >= timePerTick)
                {
                    // sufficient time has past since last tick, so we update
                    lastTick = now;
                    balance += delta - timePerTick;

                    if(balance.count() > mUpdateTiming.SecondsPerUpdate() * 5)
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
        catch(const std::exception& e)
        {
            logger()->error("Exception thrown during runtime: {}", e.what());
            return -1;
        }

        try
        {
            this->State(EState::Finalizing);
            Cleanup();
            BaseCleanupVulkan();
            BaseCleanupSdlSubsystem();
        }
        catch(const std::exception& e)
        {
            logger()->error("Exception thrown during deconstruct: {}", e.what());
            return -1;
        }

        this->State(EState::Uninitialized);
        return 0;
    }

    void MinimalAppBase::BaseInitSdlSubsystem() { mOsManager.Init(); }

    void MinimalAppBase::BaseInit()
    {
        // print validation layer messages with the logger
        mVkbInstanceBuilder.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32 {
            auto severity = vkb::to_string_message_severity(messageSeverity);
            auto type     = vkb::to_string_message_type(messageType);
            switch(messageSeverity)
            {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    logger()->info("[{}: {}] {}", severity, type, pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    logger()->info("[{}: {}] {}", severity, type, pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    logger()->warn("[{}: {}] {}", severity, type, pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    logger()->error("[{}: {}] {}", severity, type, pCallbackData->pMessage);
                    break;
            }
            return VK_FALSE;
        });

        if(mEnableDefaultValidationLayers)
        {
            mVkbInstanceBuilder.enable_layer("VK_LAYER_KHRONOS_validation");
            mVkbInstanceBuilder.enable_validation_layers();
        }

        auto instanceBuildRet = mVkbInstanceBuilder.build();

        if(!instanceBuildRet)
        {
            logger()->error("Create vkInst failed: {}", instanceBuildRet.error().message());
            throw std::exception();
        }

        mInstanceVkb = instanceBuildRet.value();
        mInstance    = mInstanceVkb.instance;
    }
    void MinimalAppBase::BasePollEvents()
    {
        for(Event::ptr event = mOsManager.PollEvent(); event != nullptr; event = mOsManager.PollEvent())
        {
            OnEvent(event);
            if(event->Source())
            {
                if(event->Type() == Event::EType::WindowCloseRequested && Window::Windows().size() == 1)
                {
                    State(EState::StopRequested);
                }
            }
        }
    }
    void MinimalAppBase::BaseCleanupVulkan()
    {
        vkb::destroy_instance(mInstanceVkb);
        mInstance = nullptr;
    }
    void MinimalAppBase::BaseCleanupSdlSubsystem()
    {
        mOsManager.Cleanup();
        logger()->flush();
    }

    void MinimalAppBase::PrintStateChange(EState oldState, EState newState)
    {
        logger()->info("{} => {}", NAMEOF_ENUM(oldState), NAMEOF_ENUM(newState));
    }
}  // namespace hsk