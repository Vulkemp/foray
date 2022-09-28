#pragma once
#include "hsk_basesbt.hpp"
#include "hsk_rtshadertypes.hpp"

namespace hsk {
    /// @brief Shader binding table for storing raygen, miss and callable shader groups
    class GeneralShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        /// @brief General shader group
        struct ShaderGroup
        {
            ShaderModule* Module = nullptr;
        };

        explicit GeneralShaderBindingTable(RtShaderGroupType groupType, VkDeviceSize entryDataSize = 0);

        /// @brief Set shader group with shader
        void SetGroup(GroupIndex groupIndex, ShaderModule* shader);
        /// @brief Set shader group with shader and custom data
        /// @param data pointer to a memory area of mEntryDataSize size. Use SetShaderDataSize(...) before adding shaders!
        void SetGroup(GroupIndex groupIndex, ShaderModule* shader, const void* data);

        HSK_PROPERTY_CGET(Groups)
        HSK_PROPERTY_CGET(ShaderGroupType)

        virtual void        WriteToShaderCollection(RtShaderCollection& collection) const;
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const;

        virtual void Destroy() override;

      protected:
        RtShaderGroupType        mShaderGroupType = RtShaderGroupType::Undefined;
        std::vector<ShaderGroup> mGroups;

        inline virtual size_t GetGroupArrayCount() const { return mGroups.size(); }
    };
}  // namespace hsk
