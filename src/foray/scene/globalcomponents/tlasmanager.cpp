#include "tlasmanager.hpp"
#include "../components/meshinstance.hpp"
#include "../scene.hpp"
#include <unordered_set>

namespace foray::scene::gcomp {

    void TlasManager::CreateOrUpdate()
    {
        mBlasInstanceIds.clear();
        mMeshInstances.clear();

        as::Tlas::Builder builder;
        if (mTlas)
        {
            builder.SetAccelerationStructure(mTlas->GetAccelerationStructure());
        }
        std::vector<Node*> nodesWithMeshInstances;
        GetScene()->FindNodesWithComponent<ncomp::MeshInstance>(nodesWithMeshInstances);

        for(auto node : nodesWithMeshInstances)
        {
            auto     meshInstance = node->GetComponent<ncomp::MeshInstance>();
            uint64_t key;
            builder.AddBlasInstance(meshInstance, key);
            mBlasInstanceIds[meshInstance] = key;
            mMeshInstances[key]            = meshInstance;
        }

        mTlas.New(GetContext(), builder);
    }
    void TlasManager::Update(SceneUpdateInfo& updateInfo)
    {
        mTlas->CmdUpdate(updateInfo.CmdBuffer, updateInfo.RenderInfo.GetFrameNumber());
    }

}  // namespace foray::scene::gcomp