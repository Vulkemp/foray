#include "framerenderinfo.hpp"

namespace foray::base{
    core::RenderTargetState& FrameRenderInfo::RegisterRenderTarget(std::string_view name, core::IRenderTarget* frameBuffer)
    {
        std::string keycopy(name);
        return (mRenderTargetViews[keycopy] = core::RenderTargetState(frameBuffer));
    }
    core::RenderTargetState& FrameRenderInfo::GetRenderTargetView(std::string_view name)
    {
        std::string keycopy(name);
        return mRenderTargetViews[keycopy];
    }
}  // namespace foray::base