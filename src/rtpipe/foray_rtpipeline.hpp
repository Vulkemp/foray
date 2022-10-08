#pragma once
#include "../foray_basics.hpp"
#include "foray_generalsbt.hpp"
#include "foray_hitsbt.hpp"
#include "foray_rtshadercollection.hpp"

namespace foray::rtpipe {

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

        FORAY_PROPERTY_ALLGET(RaygenSbt)
        FORAY_PROPERTY_ALLGET(HitSbt)
        FORAY_PROPERTY_ALLGET(MissSbt)
        FORAY_PROPERTY_ALLGET(CallablesSbt)
        FORAY_PROPERTY_ALLGET(Pipeline)
        FORAY_PROPERTY_ALLGET(PipelineLayout)

        /// @brief Builds RtPipeline with shaders and shadergroups as defined in Sbt wrappers and builds Sbts.
        void Build(const core::VkContext* context, VkPipelineLayout pipelineLayout);

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
}  // namespace foray