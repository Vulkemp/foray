#include "hsk_meshinstance.hpp"
#include "../globalcomponents/hsk_geometrystore.hpp"
#include "../globalcomponents/hsk_scenetransformbuffer.hpp"
#include "../hsk_node.hpp"
#include "hsk_transform.hpp"

namespace hsk {
    void NMeshInstance::BeforeDraw(const FrameRenderInfo& renderInfo)
    {
        // Push Transform to SceneTransformState
        auto transform       = GetNode()->GetTransform();
        auto transformBuffer = GetGlobals()->GetComponent<SceneTransformBuffer>();
        transformBuffer->UpdateSceneTransform(mInstanceIndex, transform->GetGlobalMatrix());
    }
    void NMeshInstance::Draw(SceneDrawInfo& drawInfo)
    {
        Mesh*          mesh;
        GeometryStore* geoStore = GetGlobals()->GetComponent<GeometryStore>();
        if(geoStore && mMeshIndex >= 0 && mMeshIndex < geoStore->GetMeshes().size())
        {
            mesh = geoStore->GetMeshes()[mMeshIndex].get();
        }
        if(mesh)
        {
            drawInfo.CmdPushConstant(mInstanceIndex);
            mesh->CmdDraw(drawInfo.RenderInfo.GetCommandBuffer(), drawInfo.CurrentlyBoundGeoBuffers);
        }
    }
}  // namespace hsk