#include "foray_callbackdispatcher.hpp"

namespace foray::scene {
    void CallbackDispatcher::InvokeUpdate(SceneUpdateInfo& updateInfo)
    {
        mUpdate.Invoke(updateInfo);
    }
    void CallbackDispatcher::InvokeDraw(SceneDrawInfo& drawInfo)
    {
        mDraw.Invoke(drawInfo);
    }
    void CallbackDispatcher::InvokeOnEvent(const Event* event)
    {
        mOnEvent.Invoke(event);
    }
    void CallbackDispatcher::InvokeOnResized(VkExtent2D extent)
    {
        mOnResized.Invoke(extent);
    }
}  // namespace foray