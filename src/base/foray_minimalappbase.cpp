#include "foray_minimalappbase.hpp"

#include "../foray_logger.hpp"
#include "../osi/foray_osi_event.hpp"
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
        while (mOsManager.PollEvent()) ;
    }

    void MinimalAppBase::OnOsEvent(const osi::Event* event)
    {
        ApiOnOsEvent(event);
        if (event->Type == osi::Event::EType::WindowCloseRequested && osi::Window::Windows().size() == 1)
        {
            mRenderLoop.RequestStop();
        }
    }
    void MinimalAppBase::Destroy()
    {
        mInstance.Destroy();
        mOsManager.Destroy();
        logger()->flush();
    }
}  // namespace foray::base