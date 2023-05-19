#pragma once
#include "foray_raytracingstage.hpp"

namespace foray::stages {
    /// @brief Extended version of MinimalRaytracingStageBase limited to a single output image, descriptorset but providing
    /// built in support for scene (Camera, Tlas, Geometry, Materials), EnvironmentMap and Noise Texture
    /// @details
    /// # Features
    ///  * Fully setup descriptorset
    ///  * Pipeline Barriers
    ///  * Binding Pipeline and DescriptorSet
    ///  * Automatically resized output image
    ///  * uint (32bit) seed value provided via push constant (offset adjustable via mRngSeedPushCOffset. Disable entirely by setting to ~0U)
    /// # Inheriting
    ///  * Required Overrides: ApiCreateRtPipeline(), ApiDestroyRtPipeline()
    class DefaultRaytracingStageBase : public RaytracingStageBase
    {
    public:
        /// @brief Image Output of the Raytracing Stage
        inline static constexpr std::string_view OutputName = "Rt.Output";

        inline core::ManagedImage* GetRtOutput() { return &mOutput; }

    protected:
        void CreateOutputImages() override;

        void CreateOrUpdateDescriptors() override;

        void RecordFrameBarriers(VkCommandBuffer cmdBuffer, base::FrameRenderInfo &renderInfo, std::vector<VkImageMemoryBarrier2> &imageFullBarriers,
                                 std::vector<VkImageMemoryBarrier2> &imageByRegionBarriers, std::vector<VkBufferMemoryBarrier2> &bufferBarriers) override;

        void RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo &renderInfo) override;

        /// @brief Image output
        core::ManagedImage mOutput;
    };
}  // namespace foray::stages