#include "minimalappbase.hpp"
#include "applicationloop.hpp"
#include "../logger.hpp"
#include "../osi/osi_event.hpp"
#include "../osi/window.hpp"
#include <nameof/nameof.hpp>

namespace foray::base {

    void lPrintStateChange(ELifetimeState oldState, ELifetimeState newState)
    {
        logger()->info("Lifetime State: {} => {}", NAMEOF_ENUM(oldState), NAMEOF_ENUM(newState));
    }

    MinimalAppBase::MinimalAppBase(AppLoopBase* apploop)
        : mAppLoop(apploop)
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

    void MinimalAppBase::IApplicationInit()
    {
        mOsManager.Init();
        mInstance->Create();
    }

    void MinimalAppBase::IApplicationProcessEvents()
    {
        while (mOsManager.PollEvent()) ;
    }

    void MinimalAppBase::OnOsEvent(const osi::Event* event)
    {
        ApiOnOsEvent(event);
        if (event->Type == osi::Event::EType::WindowCloseRequested && osi::Window::Windows().size() == 1)
        {
            mAppLoop->RequestStop();
        }
    }
    MinimalAppBase::~MinimalAppBase()
    {
        mOsEventReceiver.Destroy();
        mInstance = nullptr;
        mOsManager.Destroy();
        logger()->flush();
    }
}  // namespace foray::base