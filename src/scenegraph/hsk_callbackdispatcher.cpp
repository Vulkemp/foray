#include "hsk_callbackdispatcher.hpp"

namespace hsk {
    void CallbackDispatcher::InvokeUpdate(const FrameUpdateInfo& updateInfo)
    {
        mUpdate.Invoke(updateInfo);
    }
    void CallbackDispatcher::InvokeBeforeDraw(const FrameRenderInfo& renderInfo)
    {
        mBeforeDraw.Invoke(renderInfo);
    }
    void CallbackDispatcher::InvokeDraw(SceneDrawInfo& drawInfo)
    {
        mDraw.Invoke(drawInfo);
    }
    void CallbackDispatcher::InvokeOnEvent(std::shared_ptr<Event> event)
    {
        mOnEvent.Invoke(event);
    }
    void CallbackDispatcher::InvokeOnResized(VkExtent2D extent)
    {
        mOnResized.Invoke(extent);
    }
}  // namespace hsk