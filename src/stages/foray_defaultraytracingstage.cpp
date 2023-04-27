#include "foray_defaultraytracingstage.hpp"

namespace foray::stages {
    void DefaultRaytracingStageBase::CreateOutputImages() {
        VkImageUsageFlags imageUsageFlags =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT;

        mOutput.Create(mContext, imageUsageFlags, VK_FORMAT_R16G16B16A16_SFLOAT, mContext->GetSwapchainSize(), OutputName);
        mImageOutputs[std::string(OutputName)] = &mOutput;
    }

    void DefaultRaytracingStageBase::CreateOrUpdateDescriptors() {
        using namespace rtbindpoints;

        mDescriptorSet.SetDescriptorAt(BIND_OUT_IMAGE, mOutput, VK_IMAGE_LAYOUT_GENERAL, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, RTSTAGEFLAGS);
        RaytracingStageBase::CreateOrUpdateDescriptors();
    }

    void DefaultRaytracingStageBase::RecordFrameBarriers(VkCommandBuffer cmdBuffer, base::FrameRenderInfo &renderInfo, std::vector<VkImageMemoryBarrier2> &imageBarriers,
                                                         std::vector<VkBufferMemoryBarrier2> &bufferBarriers) {
        RaytracingStageBase::RecordFrameBarriers(cmdBuffer, renderInfo, imageBarriers, bufferBarriers);

        imageBarriers.push_back(renderInfo.GetImageLayoutCache().MakeBarrier(mOutput, core::ImageLayoutCache::Barrier2{
                .SrcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .SrcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                .DstStageMask  = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                .DstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .NewLayout     = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL
        }));
    }

    void DefaultRaytracingStageBase::RecordFrameTraceRays(VkCommandBuffer cmdBuffer, base::FrameRenderInfo &renderInfo) {
        VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry = mPipeline.GetRaygenSbt().GetAddressRegion();
        VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry = mPipeline.GetMissSbt().GetAddressRegion();
        VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry = mPipeline.GetHitSbt().GetAddressRegion();
        VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry = mPipeline.GetCallablesSbt().GetAddressRegion();
        VkExtent2D extent{mOutput.GetExtent3D().width, mOutput.GetExtent3D().height};
        mContext->VkbDispatchTable->cmdTraceRaysKHR(cmdBuffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry, &hit_shader_sbt_entry, &callable_shader_sbt_entry,
                                                    extent.width, extent.height, 1U);
    }
}
