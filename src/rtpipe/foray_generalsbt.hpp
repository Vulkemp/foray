#pragma once
#include "foray_basesbt.hpp"
#include "foray_rtshadertypes.hpp"
#include "../core/foray_shadermodule.hpp"
#include <vector>

namespace foray::rtpipe {
    /// @brief Shader binding table for storing raygen, miss and callable shader groups
    class GeneralShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        /// @brief General shader group
        struct ShaderGroup
        {
            core::ShaderModule* Module = nullptr;
        };

        explicit GeneralShaderBindingTable(RtShaderGroupType groupType, VkDeviceSize entryDataSize = 0);

        /// @brief Set shader group with shader
        void SetGroup(GroupIndex groupIndex, core::ShaderModule* shader);
        /// @brief Set shader group with shader and custom data
        /// @param data pointer to a memory area of mEntryDataSize size. Use SetShaderDataSize(...) before adding shaders!
        void SetGroup(GroupIndex groupIndex, core::ShaderModule* shader, const void* data);

        FORAY_PROPERTY_CGET(Groups)
        FORAY_PROPERTY_CGET(ShaderGroupType)

        virtual void        WriteToShaderCollection(RtShaderCollection& collection) const override;
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const override;

        virtual void Destroy() override;

      protected:
        RtShaderGroupType              mShaderGroupType = RtShaderGroupType::Undefined;
        std::vector<ShaderGroup> mGroups;

        inline virtual size_t GetGroupArrayCount() const override { return mGroups.size(); }
    };
}  // namespace foray::rtpipe
