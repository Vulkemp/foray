#include "hsk_meshInstance.hpp"
#include "../hsk_node.hpp"

namespace hsk {
    void NMeshInstance::BeforeDraw(const FrameRenderInfo& renderInfo)
    {
        // TODO: Push Transform to SceneTransformState
        auto transform = GetNode()->GetTransform();
    }
    void NMeshInstance::Draw(SceneDrawInfo& drawInfo)
    {
        drawInfo.CmdPushConstant(mIndex);
        for(auto& primitive : mPrimitives)
        {
            primitive.Draw(drawInfo.RenderInfo.GetCommandBuffer());
        }
    }
}  // namespace hsk