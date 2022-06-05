#include "hsk_meshInstance.hpp"
#include "../hsk_node.hpp"
#include "hsk_transform.hpp"
#include "../globalcomponents/hsk_scenetransformbuffer.hpp"

namespace hsk {
    void NMeshInstance::BeforeDraw(const FrameRenderInfo& renderInfo)
    {
        // Push Transform to SceneTransformState
        auto transform = GetNode()->GetTransform();
        auto transformBuffer = GetGlobals()->GetComponent<SceneTransformBuffer>();
        transformBuffer->UpdateSceneTransform(mIndex, transform->GetGlobalMatrix());
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