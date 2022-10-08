#pragma once
#include "../../as/foray_blas.hpp"
#include "../../as/foray_tlas.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"
#include <unordered_map>

namespace foray::scene {
    class TlasManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        explicit TlasManager(const core::VkContext* context);

        void CreateOrUpdate();

        void Update(const base::FrameUpdateInfo& updateInfo);

        FORAY_PROPERTY_ALLGET(Tlas)
        FORAY_PROPERTY_ALLGET(MeshInstances)
        FORAY_PROPERTY_ALLGET(BlasInstanceIds)

      private:
        as::Tlas                                    mTlas;
        std::unordered_map<uint64_t, MeshInstance*> mMeshInstances;
        std::unordered_map<MeshInstance*, uint64_t> mBlasInstanceIds;
    };
}  // namespace foray::scene