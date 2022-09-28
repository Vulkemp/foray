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

        void AddShaderGroupRaygen(ShaderModule* module);
        void AddShaderGroupCallable(ShaderModule* module);
        void AddShaderGroupMiss(ShaderModule* module);
        void AddShaderGroupIntersect(ShaderModule* closestHit, ShaderModule* anyHit, ShaderModule* intersect);

        void Build(const VkContext* context);

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