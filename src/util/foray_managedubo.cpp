#include "foray_managedubo.hpp"
#include "../foray_vma.hpp"

namespace foray::util {
    void ManagedUboBase::Create(core::Context* context, VkDeviceSize size, uint32_t stageBufferCount)
    {
        Destroy();
        core::ManagedBuffer::CreateInfo ci(VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, size,
                                                        VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mUboBuffer.Create(context, ci, stageBufferCount);
    }
    void ManagedUboBase::CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer)
    {
        mUboBuffer.CmdCopyToDevice(frameIndex, cmdBuffer);
    }
    void ManagedUboBase::CmdPrepareForRead(VkCommandBuffer cmdBuffer, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) const
    {
        mUboBuffer.CmdPrepareForRead(cmdBuffer, dstStageMask, dstAccessMask);
    }
    VkBufferMemoryBarrier2 ManagedUboBase::MakeBarrierPrepareForRead(VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) const
    {
        return mUboBuffer.MakeBarrierPrepareForRead(dstStageMask, dstAccessMask);
    }
    VkDescriptorBufferInfo ManagedUboBase::GetVkDescriptorBufferInfo() const
    {
        return mUboBuffer.GetVkDescriptorInfo();
    }
    bool ManagedUboBase::Exists() const
    {
        return mUboBuffer.Exists();
    }
    void ManagedUboBase::Destroy()
    {
        mUboBuffer.Destroy();
    }

}  // namespace foray::util