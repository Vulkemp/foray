#include "hsk_tlasmanager.hpp"
#include "../hsk_scene.hpp"
#include <unordered_set>
#include "../components/hsk_meshinstance.hpp"

namespace hsk {
    TlasManager::TlasManager(const VkContext* context) : mTlas(context) {}

    void TlasManager::CreateOrUpdate()
    {
        mTlas.ClearBlasInstances();
        mBlasInstanceIds.clear();
        mMeshInstances.clear();

        std::vector<Node*> nodesWithMeshInstances;
        GetScene()->FindNodesWithComponent<MeshInstance>(nodesWithMeshInstances);

        for(auto node : nodesWithMeshInstances)
        {
            auto     meshInstance          = node->GetComponent<MeshInstance>();
            uint64_t id                    = mTlas.AddBlasInstanceAuto(meshInstance);
            mBlasInstanceIds[meshInstance] = id;
            mMeshInstances[id]             = meshInstance;
        }

        mTlas.CreateOrUpdate();
    }
    void TlasManager::Update(const FrameUpdateInfo& updateInfo) 
    {
        mTlas.UpdateLean(updateInfo.GetCommandBuffer(), updateInfo.GetFrameNumber());
    }

}  // namespace hsk