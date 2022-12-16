#pragma once
#include "../foray_basics.hpp"
#include "foray_generalsbt.hpp"
#include "foray_hitsbt.hpp"
#include "foray_rtshadercollection.hpp"

namespace foray::rtpipe {

    /// @brief Helper class wrapping 4 SBTs (Raygen, Miss, Callable, Hitgroup) and a raytracing pipeline
    /// @details
    /// Usage:
    ///   1. Configure Sbts, by accessing them via GetRaygenSbt(), ... . Refer to ShaderBindingTableBase class definition for more information on shader binding tables.
    ///   2. Call Build(...) to build the Pipeline and Sbts
    class RtPipeline
    {
      public:
        RtPipeline();

        FORAY_GETTER_R(RaygenSbt)
        FORAY_GETTER_R(HitSbt)
        FORAY_GETTER_R(MissSbt)
        FORAY_GETTER_R(CallablesSbt)
        FORAY_GETTER_V(Pipeline)
        FORAY_PROPERTY_V(PipelineLayout)

        /// @brief Builds RtPipeline with shaders and shadergroups as defined in Sbt wrappers and builds Sbts.
        void Build(core::Context* context, VkPipelineLayout pipelineLayout);

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

        core::Context* mContext = nullptr;
    };
}  // namespace foray::rtpipe