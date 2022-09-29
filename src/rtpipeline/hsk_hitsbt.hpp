#pragma once
#include "hsk_basesbt.hpp"
#include "hsk_rtshadertypes.hpp"

namespace hsk {
    class HitShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        struct ShaderGroup
        {
            ShaderModule* ClosestHitModule = nullptr;
            ShaderModule* AnyHitModule     = nullptr;
            ShaderModule* IntersectModule  = nullptr;
        };

        inline explicit HitShaderBindingTable(VkDeviceSize entryDataSize = 0) : ShaderBindingTableBase(entryDataSize) {}

        /// @brief Set shader group with shader
        void SetGroup(GroupIndex groupIndex, ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect);
        /// @brief Set shader group with shader and custom data
        /// @param data pointer to a memory area of mEntryDataSize size. Use SetShaderDataSize(...) before adding shaders!
        void SetGroup(GroupIndex groupIndex, ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect, const void* data);

        virtual void        WriteToShaderCollection(RtShaderCollection& collection) const override;
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const override;


        HSK_PROPERTY_CGET(Groups)

        virtual void Destroy() override;

      protected:
        std::vector<ShaderGroup> mGroups;

        inline virtual size_t GetGroupArrayCount() const override { return mGroups.size(); }
    };

}  // namespace hsk
