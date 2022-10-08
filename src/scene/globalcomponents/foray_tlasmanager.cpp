#include "foray_tlasmanager.hpp"
#include "../components/foray_meshinstance.hpp"
#include "../foray_scene.hpp"
#include <unordered_set>

namespace foray::scene {
    TlasManager::TlasManager(const core::VkContext* context) : mTlas(context) {}

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
    void TlasManager::Update(const base::FrameUpdateInfo& updateInfo)
    {
        mTlas.UpdateLean(updateInfo.GetCommandBuffer(), updateInfo.GetFrameNumber());
    }

}  // namespace foray::scene