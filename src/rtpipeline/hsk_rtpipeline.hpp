#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "hsk_generalsbt.hpp"
#include "hsk_intersectsbt.hpp"
#include "hsk_rtshadercollection.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace hsk {

    class RtPipeline
    {
      public:
        HSK_PROPERTY_ALLGET(RaygenSbt)
        HSK_PROPERTY_ALLGET(IntersectsSbt)
        HSK_PROPERTY_ALLGET(MissSbt)
        HSK_PROPERTY_ALLGET(CallablesSbt)
        HSK_PROPERTY_ALLGET(Pipeline)
        HSK_PROPERTY_ALL(PipelineLayout)

        uint32_t AddShaderGroupRaygen(ShaderModule* module);
        uint32_t AddShaderGroupCallable(ShaderModule* module);
        uint32_t AddShaderGroupMiss(ShaderModule* module);
        uint32_t AddShaderGroupIntersect(ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect);

        void Build(const VkContext* context);

        // struct ShaderGroup
        // {
        //     ShaderGroupId     Id         = {};
        //     RtShaderGroupType Type       = RtShaderGroupType::Undefined;
        //     ShaderModule*     General    = nullptr;
        //     ShaderModule*     ClosestHit = nullptr;
        //     ShaderModule*     AnyHit     = nullptr;
        //     ShaderModule*     Intersect  = nullptr;

        //     VkRayTracingShaderGroupCreateInfoKHR BuildShaderGroupCi(const RtShaderCollection& shaderCollection) const;
        // };

      protected:
        GeneralShaderBindingTable   mRaygenSbt;
        GeneralShaderBindingTable   mMissSbt;
        GeneralShaderBindingTable   mCallablesSbt;
        IntersectShaderBindingTable mIntersectsSbt;

        // std::vector<ShaderGroup> mShaderGroups;

        RtShaderCollection mShaderCollection;

        VkPipelineLayout mPipelineLayout;
        VkPipeline       mPipeline;
    };
}  // namespace hsk