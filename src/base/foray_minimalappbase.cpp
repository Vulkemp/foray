#include "foray_minimalappbase.hpp"

#include "../foray_logger.hpp"
#include "../osi/foray_event.hpp"
#include "../osi/foray_window.hpp"
#include <nameof/nameof.hpp>

namespace foray::base {

    void lPrintStateChange(ELifetimeState oldState, ELifetimeState newState)
    {
        logger()->info("Lifetime State: {} => {}", NAMEOF_ENUM(oldState), NAMEOF_ENUM(newState));
    }

    MinimalAppBase::MinimalAppBase(bool printStateChanges)
        : mRenderLoop([this]() { this->Init(); },
                      [this](RenderLoop::RenderInfo& renderInfo) { this->ApiRender(renderInfo); },
                      [this]() { return this->ApiCanRenderNextFrame(); },
                      [this]() { this->Destroy(); },
                      [this]() { this->PollEvents(); },
                      (printStateChanges ? &PrintStateChange : nullptr))
        , mOsManager()
        , mInstance(
              &mContext,
              [this](vkb::InstanceBuilder& builder) { this->ApiBeforeInstanceCreate(builder); },
#if FORAY_DEBUG || FORAY_VALIDATION  // Set validation layers and debug callbacks on / off
              true
#else
              false
#endif
          )
    {
    }

    int32_t MinimalAppBase::Run()
    {
        return mRenderLoop.Run();
    }

    void MinimalAppBase::Init()
    {
        mOsManager.Init();
        mInstance.Create();
    }

    void MinimalAppBase::PollEvents()
    {
        osi::OsManager::EventPollResult pollResult;
        for(pollResult = mOsManager.PollEvent(); pollResult.Any; pollResult = mOsManager.PollEvent())
        {
            ApiOnEvent(&pollResult.Raw);
            if(!!pollResult.Cast)
            {
                ApiOnEvent(pollResult.Cast);
                if(pollResult.Cast->Source && pollResult.Cast->Type == osi::Event::EType::WindowCloseRequested && osi::Window::Windows().size() <= 1)
                {
                    // The last window has been requested to close, oblige by stopping the renderloop
                    mRenderLoop.RequestStop();
                }
            }
        }
    }
    void MinimalAppBase::Destroy()
    {
        mInstance.Destroy();
        mOsManager.Destroy();
        logger()->flush();
    }
}  // namespace foray::base