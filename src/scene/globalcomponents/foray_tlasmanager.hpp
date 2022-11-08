#pragma once
#include "../../as/foray_blas.hpp"
#include "../../as/foray_tlas.hpp"
#include "../foray_component.hpp"
#include "../foray_scene_declares.hpp"
#include <unordered_map>

namespace foray::scene::gcomp {
    /// @brief Manages a Tlas for a scene
    class TlasManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        TlasManager() = default;

        void CreateOrUpdate();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        virtual int32_t GetOrder() const override { return ORDER_DEVICEUPLOAD; }

        FORAY_GETTER_CR(Tlas)
        FORAY_GETTER_CR(MeshInstances)
        FORAY_GETTER_CR(BlasInstanceIds)

      private:
        as::Tlas                                    mTlas;
        std::unordered_map<uint64_t, ncomp::MeshInstance*> mMeshInstances;
        std::unordered_map<ncomp::MeshInstance*, uint64_t> mBlasInstanceIds;
    };
}  // namespace foray::scene