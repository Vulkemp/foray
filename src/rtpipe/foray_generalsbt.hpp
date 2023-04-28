#pragma once
#include "../core/foray_shadermodule.hpp"
#include "foray_basesbt.hpp"
#include "foray_rtshadertypes.hpp"
#include <vector>

namespace foray::rtpipe {
    /// @brief Shader binding table for storing raygen, miss and callable shader groups
    class GeneralShaderBindingTable : public ShaderBindingTableBase
    {
      public:
        class Builder : public ShaderBindingTableBase::Builder
        {
          public:
            Builder() = default;
            inline Builder(RtShaderGroupType groupType) : mGroupType(groupType){}

            Builder& SetEntry(int32_t groupIdx, core::ShaderModule* module, const void* data = nullptr, std::size_t size = 0)
            {
                SetEntryModule(groupIdx, module);
                if(!!data && size > 0)
                {
                    SetEntryData(groupIdx, data, size);
                }
                return *this;
            }
            template <typename T>
            Builder& SetEntry(int32_t groupIdx, core::ShaderModule* module, const T& data)
            {
                return SetEntry(groupIdx, module, &data, sizeof(T));
            }
            core::ShaderModule* GetEntryModule(int32_t groupIdx)
            {
                Assert(groupIdx >= 0 && groupIdx < (int32_t)mModules.size(), "Group Index out of range");
                return mModules[groupIdx];
            }
            Builder& SetEntryModule(int32_t groupIdx, core::ShaderModule* module);

            FORAY_PROPERTY_V(GroupType)
            FORAY_PROPERTY_R(Modules)

            void        WriteToShaderCollection(RtShaderCollection& collection) const;
            VectorRange WriteToShaderGroupCiVector(std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groupCis, const RtShaderCollection& shaderCollection) const;

          private:
            RtShaderGroupType                mGroupType;
            std::vector<core::ShaderModule*> mModules;
        };

        GeneralShaderBindingTable(core::Context* context, const Builder& builder);
   };
}  // namespace foray::rtpipe
