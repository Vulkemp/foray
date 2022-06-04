#include "hsk_rootregistry.hpp"

namespace hsk {
    void RootRegistry::InvokeUpdate(const FrameUpdateInfo& updateInfo) { mUpdate.Invoke(updateInfo); }
    void RootRegistry::InvokeBeforeDraw(const FrameRenderInfo& renderInfo) { mBeforeDraw.Invoke(renderInfo); }
    void RootRegistry::InvokeDraw(SceneDrawInfo& drawInfo) { mDraw.Invoke(drawInfo); }
    void RootRegistry::InvokeOnEvent(std::shared_ptr<Event> event) { mOnEvent.Invoke(event); }
}  // namespace hsk