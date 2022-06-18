#include "hsk_meshinstance.hpp"
#include "../globalcomponents/hsk_geometrystore.hpp"
#include "../globalcomponents/hsk_scenetransformbuffer.hpp"
#include "../hsk_node.hpp"
#include "hsk_transform.hpp"

namespace hsk {
    void MeshInstance::BeforeDraw(const FrameRenderInfo& renderInfo)
    {
        // Push Transform to SceneTransformState
        auto transform       = GetNode()->GetTransform();
        auto transformBuffer = GetGlobals()->GetComponent<SceneTransformBuffer>();
        transformBuffer->UpdateSceneTransform(mInstanceIndex, transform->GetGlobalMatrix());
    }
    void MeshInstance::Draw(SceneDrawInfo& drawInfo)
    {
        if(mMesh)
        {
            drawInfo.CmdPushConstant(mInstanceIndex);
            mMesh->CmdDraw(drawInfo.RenderInfo.GetCommandBuffer(), drawInfo.CurrentlyBoundGeoBuffers);
        }
    }
}  // namespace hsk