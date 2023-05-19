#pragma once
#include "../basics.hpp"
#include "generalsbt.hpp"
#include "hitsbt.hpp"
#include "rtshadercollection.hpp"

namespace foray::rtpipe {

    /// @brief Helper class wrapping 4 SBTs (Raygen, Miss, Callable, Hitgroup) and a raytracing pipeline
    /// @details
    /// Usage:
    ///   1. Configure Sbts, by accessing them via GetRaygenSbt(), ... . Refer to ShaderBindingTableBase class definition for more information on shader binding tables.
    ///   2. Call Build(...) to build the Pipeline and Sbts
    class RtPipeline
    {
      public:
        class Builder
        {
          public:
            FORAY_PROPERTY_R(RaygenSbtBuilder)
            FORAY_PROPERTY_R(MissSbtBuilder)
            FORAY_PROPERTY_R(CallableSbtBuilder)
            FORAY_PROPERTY_R(HitSbtBuilder)
            FORAY_PROPERTY_V(PipelineLayout)
          private:
            GeneralShaderBindingTable::Builder mRaygenSbtBuilder   = RtShaderGroupType::Raygen;
            GeneralShaderBindingTable::Builder mMissSbtBuilder     = RtShaderGroupType::Miss;
            GeneralShaderBindingTable::Builder mCallableSbtBuilder = RtShaderGroupType::Callable;
            HitShaderBindingTable::Builder     mHitSbtBuilder;  // no assignment necessary
            VkPipelineLayout                   mPipelineLayout = nullptr;
        };

      public:
        FORAY_GETTER_R(RaygenSbt)
        FORAY_GETTER_R(HitSbt)
        FORAY_GETTER_R(MissSbt)
        FORAY_GETTER_R(CallablesSbt)
        FORAY_GETTER_V(Pipeline)
        FORAY_GETTER_V(PipelineLayout)

        /// @brief Builds RtPipeline with shaders and shadergroups as defined in Sbt wrappers and builds Sbts.
        RtPipeline(core::Context* context, const Builder& builder);

        /// @brief vkCmdBindPipeline(cmdBuffer, RayTracingBindPoint, mPipeline)
        void CmdBindPipeline(VkCommandBuffer cmdBuffer) const;

        void CmdTraceRays(VkCommandBuffer cmdBuffer, VkExtent3D launchSize) const;
        void CmdTraceRays(VkCommandBuffer cmdBuffer, VkExtent2D launchSize) const { CmdTraceRays(cmdBuffer, VkExtent3D{launchSize.width, launchSize.height, 1}); }

        virtual ~RtPipeline();

      protected:
        core::Context* mContext = nullptr;

        Local<GeneralShaderBindingTable> mRaygenSbt;
        Local<GeneralShaderBindingTable> mMissSbt;
        Local<GeneralShaderBindingTable> mCallablesSbt;
        Local<HitShaderBindingTable>     mHitSbt;

        VkPipelineLayout mPipelineLayout = nullptr;
        VkPipeline       mPipeline       = nullptr;
    };
}  // namespace foray::rtpipe