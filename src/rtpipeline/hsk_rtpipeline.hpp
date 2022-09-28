#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "hsk_generalsbt.hpp"
#include "hsk_hitsbt.hpp"
#include "hsk_rtshadercollection.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace hsk {

    /// @brief Helper class wrapping 4 SBTs (Raygen, Miss, Callable, Hitgroup) and a raytracing pipeline
    class RtPipeline
    {
      /*
        Usage:
          1. Configure Sbts, by accessing them via GetRaygenSbt(), ... . Refer to ShaderBindingTableBase class definition.
          2. Call Build(...) to build the Pipeline and Sbts
      */
      public:
        RtPipeline();

        HSK_PROPERTY_ALLGET(RaygenSbt)
        HSK_PROPERTY_ALLGET(HitSbt)
        HSK_PROPERTY_ALLGET(MissSbt)
        HSK_PROPERTY_ALLGET(CallablesSbt)
        HSK_PROPERTY_ALLGET(Pipeline)
        HSK_PROPERTY_ALLGET(PipelineLayout)

        /// @brief Builds RtPipeline with shaders and shadergroups as defined in Sbt wrappers and builds Sbts.
        void Build(const VkContext* context, VkPipelineLayout pipelineLayout);

        /// @brief vkCmdBindPipeline(cmdBuffer, RayTracingBindPoint, mPipeline)
        void CmdBindPipeline(VkCommandBuffer cmdBuffer) const;

        void Destroy();

        inline virtual ~RtPipeline() { Destroy(); }

      protected:
        GeneralShaderBindingTable mRaygenSbt;
        GeneralShaderBindingTable mMissSbt;
        GeneralShaderBindingTable mCallablesSbt;
        HitShaderBindingTable     mHitSbt;

        RtShaderCollection mShaderCollection;

        VkPipelineLayout mPipelineLayout = nullptr;
        VkPipeline       mPipeline       = nullptr;
        VkDevice         mDevice         = nullptr;
    };
}  // namespace hsk