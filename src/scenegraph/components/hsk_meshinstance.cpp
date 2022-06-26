#include "hsk_meshinstance.hpp"
#include "../globalcomponents/hsk_geometrystore.hpp"
#include "../hsk_node.hpp"
#include "hsk_transform.hpp"

namespace hsk {
    void MeshInstance::Draw(SceneDrawInfo& drawInfo)
    {
        if(mMesh)
        {
            const auto& modelWorldMatrix = GetNode()->GetTransform()->GetGlobalMatrix();
            drawInfo.CmdPushConstant(mInstanceIndex, modelWorldMatrix, mPreviousWorldMatrix);
            mMesh->CmdDraw(drawInfo.RenderInfo.GetCommandBuffer(), drawInfo.CurrentlyBoundGeoBuffers);
            
            mPreviousWorldMatrix = modelWorldMatrix;
        }
    }
}  // namespace hsk