#pragma once
#include "foray_basesbt.hpp"
#include "foray_rtshadertypes.hpp"
#include <vector>

namespace foray::rtpipe {

    /// @brief Shader binding table implementation for storing hit groups (Consist of optional closesthit, anyhit and intersect shaders)
    class HitShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        struct ShaderGroup
        {
            core::ShaderModule* ClosestHitModule = nullptr;
            core::ShaderModule* AnyHitModule     = nullptr;
            core::ShaderModule* IntersectModule  = nullptr;
        };

        inline explicit HitShaderBindingTable(VkDeviceSize entryDataSize = 0) : ShaderBindingTableBase(entryDataSize) {}

        /// @brief Set shader group with shader
        void SetGroup(GroupIndex groupIndex, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect);
        /// @brief Set shader group with shader and custom data
        /// @param data pointer to a memory area of mEntryDataSize size. Use SetShaderDataSize(...) before adding shaders!
        void SetGroup(GroupIndex groupIndex, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect, const void* data);

        virtual void        WriteToShaderCollection(RtShaderCollection& collection) const override;
        virtual VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const override;


        FORAY_GETTER_CR(Groups)

        virtual void Destroy() override;

      protected:
        std::vector<ShaderGroup> mGroups;

        inline virtual size_t GetGroupArrayCount() const override { return mGroups.size(); }
    };

}  // namespace foray
