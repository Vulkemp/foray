#include "callbackdispatcher.hpp"

namespace foray::scene {
    void CallbackDispatcher::InvokeUpdate(SceneUpdateInfo& updateInfo)
    {
        mOnUpdate.Invoke(updateInfo);
    }
    void CallbackDispatcher::InvokeDraw(SceneDrawInfo& drawInfo)
    {
        mOnDraw.Invoke(drawInfo);
    }
    void CallbackDispatcher::InvokeOnEvent(const osi::Event* event)
    {
        mOnOsEvent.Invoke(event);
    }
}  // namespace foray