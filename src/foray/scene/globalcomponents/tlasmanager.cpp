#include "tlasmanager.hpp"
#include "../components/meshinstance.hpp"
#include "../scene.hpp"
#include <unordered_set>

namespace foray::scene::gcomp {

    void TlasManager::CreateOrUpdate()
    {
        mTlas.ClearBlasInstances();
        mBlasInstanceIds.clear();
        mMeshInstances.clear();

        std::vector<Node*> nodesWithMeshInstances;
        GetScene()->FindNodesWithComponent<ncomp::MeshInstance>(nodesWithMeshInstances);

        for(auto node : nodesWithMeshInstances)
        {
            auto     meshInstance          = node->GetComponent<ncomp::MeshInstance>();
            uint64_t id                    = mTlas.AddBlasInstanceAuto(meshInstance);
            mBlasInstanceIds[meshInstance] = id;
            mMeshInstances[id]             = meshInstance;
        }

        mTlas.CreateOrUpdate(GetContext());
    }
    void TlasManager::Update(SceneUpdateInfo& updateInfo)
    {
        mTlas.UpdateLean(updateInfo.CmdBuffer, updateInfo.RenderInfo.GetFrameNumber());
    }

}  // namespace foray::scene