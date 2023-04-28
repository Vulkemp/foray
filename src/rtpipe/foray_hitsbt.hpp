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

        class Builder : public ShaderBindingTableBase::Builder
        {
          public:
            Builder& SetEntry(
                int32_t groupIdx, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect, const void* data = nullptr, std::size_t size = 0)
            {
                SetEntryModules(groupIdx, closestHit, anyHit, intersect);
                if(!!data && size > 0)
                {
                    SetEntryData(groupIdx, data, size);
                }
                return *this;
            }
            template <typename T>
            Builder& SetEntry(int32_t groupIdx, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect, const T& data)
            {
                return SetEntry(groupIdx, closestHit, anyHit, intersect, &data, sizeof(T));
            }
            ShaderGroup GetEntryModules(int32_t groupIdx)
            {
                Assert(groupIdx >= 0 && groupIdx < (int32_t)mModules.size(), "Group Index out of range");
                return mModules[groupIdx];
            }
            Builder& SetEntryModules(int32_t groupIdx, core::ShaderModule* closestHit, core::ShaderModule* anyHit, core::ShaderModule* intersect);

            FORAY_PROPERTY_R(Modules)

            void        WriteToShaderCollection(RtShaderCollection& collection) const;
            VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const;

          private:
            std::vector<ShaderGroup> mModules;
        };

        HitShaderBindingTable(core::Context* context, const Builder& builder);
    };

}  // namespace foray::rtpipe
