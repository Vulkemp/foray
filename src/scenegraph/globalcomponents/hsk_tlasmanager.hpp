#pragma once
#include "../../raytracing/hsk_blas.hpp"
#include "../../raytracing/hsk_tlas.hpp"
#include "../hsk_component.hpp"
#include "../hsk_scenegraph_declares.hpp"
#include <unordered_map>

namespace hsk {
    class TlasManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        explicit TlasManager(const VkContext* context);

        void CreateOrUpdate();

        void Update(const FrameUpdateInfo& updateInfo);

        HSK_PROPERTY_ALLGET(Tlas)
        HSK_PROPERTY_ALLGET(MeshInstances)
        HSK_PROPERTY_ALLGET(BlasInstanceIds)

      private:
        Tlas                                        mTlas;
        std::unordered_map<uint64_t, MeshInstance*> mMeshInstances;
        std::unordered_map<MeshInstance*, uint64_t> mBlasInstanceIds;
    };
}  // namespace hsk