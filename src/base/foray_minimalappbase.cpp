#include "foray_minimalappbase.hpp"

#include "../foray_logger.hpp"
#include "../osi/foray_event.hpp"
#include <nameof/nameof.hpp>

namespace foray::base {

    void lPrintStateChange(ELifetimeState oldState, ELifetimeState newState)
    {
        logger()->info("Lifetime State: {} => {}", NAMEOF_ENUM(oldState), NAMEOF_ENUM(newState));
    }

    MinimalAppBase::MinimalAppBase(bool printStateChanges)
        : mRenderLoop([this]() { this->Init(); },
                      [this](fp32_t delta) { this->ApiRender(delta); },
                      [this]() { return this->ApiCanRenderNextFrame(); },
                      [this]() { this->Destroy(); },
                      [this]() { this->PollEvents(); },
                      (printStateChanges ? &lPrintStateChange : nullptr))
        , mOsManager()
        , mInstance([this](vkb::InstanceBuilder& builder) { this->ApiBeforeInstanceCreate(builder); }, true)
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
        for(const Event* event = mOsManager.PollEvent(); event != nullptr; event = mOsManager.PollEvent())
        {
            ApiOnEvent(event);
            if(event->Source && event->Type == Event::EType::WindowCloseRequested && Window::Windows().size() <= 1)
            {
                // The last window has been requested to close, oblige by stopping the renderloop
                mRenderLoop.RequestStop();
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