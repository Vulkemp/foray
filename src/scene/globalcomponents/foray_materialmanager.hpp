#pragma once
#include "../../util/foray_managedvectorbuffer.hpp"
#include "../foray_component.hpp"
#include "../foray_material.hpp"

namespace foray::scene::gcomp {

    /// @brief Manages a device local storage buffer providing material information for rendering
    class MaterialManager : public GlobalComponent
    {
      public:
        explicit MaterialManager(core::Context* context);

        std::vector<Material>& GetVector() { return mBuffer.GetVector(); }

        /// @brief Apply changes made to the cpu local buffer to the device local buffer
        void UpdateDeviceLocal();

        virtual ~MaterialManager() = default;

        FORAY_GETTER_CR(Buffer)

        inline VkDescriptorBufferInfo GetVkDescriptorInfo() const { return mBuffer.GetBuffer()->GetVkDescriptorBufferInfo(); }
        inline VkBuffer               GetVkBuffer() const { return mBuffer.GetBuffer()->GetBuffer(); }


      protected:
        util::ManagedVectorBuffer<Material> mBuffer = {};
    };
}  // namespace foray::scene