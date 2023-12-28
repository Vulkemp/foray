#pragma once
#include "../../util/managedvectorbuffer.hpp"
#include "../component.hpp"
#include "../material.hpp"

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

        inline vk::DescriptorBufferInfo GetVkDescriptorInfo() const { return mBuffer.GetBuffer()->GetVkDescriptorBufferInfo(); }
        inline vk::Buffer               GetVkBuffer() const { return mBuffer.GetBuffer()->GetBuffer(); }


      protected:
        util::ManagedVectorBuffer<Material> mBuffer = {};
    };
}  // namespace foray::scene