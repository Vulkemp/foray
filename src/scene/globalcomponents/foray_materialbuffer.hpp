#pragma once
#include "../../core/foray_descriptorsethelper.hpp"
#include "../../util/foray_managedvectorbuffer.hpp"
#include "../foray_component.hpp"
#include "../foray_material.hpp"

namespace foray::scene {

    /// @brief Manages a device local storage buffer providing material information for rendering
    class MaterialBuffer : public GlobalComponent
    {
      public:
        explicit MaterialBuffer(const core::VkContext* context);

        std::vector<DefaultMaterialEntry>& GetVector() { return mBuffer.GetVector(); }

        /// @brief Apply changes made to the cpu local buffer to the device local buffer
        void UpdateDeviceLocal();
        void Destroy();

        std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GetDescriptorInfo(VkShaderStageFlags shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT);

        inline virtual ~MaterialBuffer() { Destroy(); }

        FORAY_PROPERTY_ALLGET(Buffer)

      protected:
        util::ManagedVectorBuffer<DefaultMaterialEntry> mBuffer = {};
        std::vector<VkDescriptorBufferInfo>       mDescriptorBufferInfos;
    };
}  // namespace foray::scene