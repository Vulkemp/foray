#pragma once
#include "../../util/dualbuffer.hpp"
#include "../components/punctuallight.hpp"
#include "../component.hpp"
#include "../lights.hpp"
#include <unordered_map>

namespace foray::scene::gcomp {
    /// @brief Maintains a list of PunctualLight components
    class LightManager : public GlobalComponent, public Component::UpdateCallback
    {
      public:
        inline LightManager() : Component::UpdateCallback(ORDER_DEVICEUPLOAD) {}

        /// @brief Collects lights and reuploads to device
        void CreateOrUpdate();

        virtual void Update(SceneUpdateInfo& updateInfo) override;

        FORAY_PROPERTY_R(Buffer)

      protected:
        std::vector<SimpleLight>                            mSimplifiedlights;
        std::unordered_map<ncomp::PunctualLight*, uint32_t> mComponentArrayBindings;
        Local<util::DualBuffer>                                    mBuffer;
    };
}  // namespace foray::scene::gcomp
