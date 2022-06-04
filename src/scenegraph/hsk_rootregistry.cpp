#include "hsk_rootregistry.hpp"

namespace hsk {
    void RootRegistry::Update(const FrameUpdateInfo& updateInfo) { mUpdate.Invoke(updateInfo); }
    void RootRegistry::BeforeDraw(const FrameRenderInfo& renderInfo) { mBeforeDraw.Invoke(renderInfo); }
    void RootRegistry::Draw(const FrameRenderInfo& renderInfo) { mDraw.Invoke(renderInfo); }
    void RootRegistry::OnEvent(std::shared_ptr<Event> event) { mOnEvent.Invoke(event); }
}  // namespace hsk