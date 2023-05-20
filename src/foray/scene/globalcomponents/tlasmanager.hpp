#pragma once
#include "../../as/blas.hpp"
#include "../../as/tlas.hpp"
#include "../component.hpp"
#include "../scene_declares.hpp"
#include <unordered_map>

namespace foray::scene::gcomp {
    /// @brief Manages a Tlas for a scene
    class TlasManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        inline TlasManager() : Component::UpdateCallback(ORDER_DEVICEUPLOAD) {}

        void CreateOrUpdate();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        FORAY_GETTER_MEM(Tlas)
        FORAY_GETTER_CR(MeshInstances)
        FORAY_GETTER_CR(BlasInstanceIds)

      private:
        Local<as::Tlas>                                    mTlas;
        std::unordered_map<uint64_t, ncomp::MeshInstance*> mMeshInstances;
        std::unordered_map<ncomp::MeshInstance*, uint64_t> mBlasInstanceIds;
    };
}  // namespace foray::scene