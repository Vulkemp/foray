#include "foray_managedubo.hpp"
#include "../foray_vma.hpp"

namespace foray::util {
    void ManagedUboBase::Create(core::Context* context, VkDeviceSize size, uint32_t stageBufferCount)
    {
        Destroy();
        core::ManagedBuffer::ManagedBufferCreateInfo ci(VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, size,
                                                  VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mUboBuffer.Create(context, ci, stageBufferCount);
    }
    void ManagedUboBase::CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer)
    {
        DualBuffer::DeviceBufferState beforeAndAfter{.AccessFlags        = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
                                                     .PipelineStageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
                                                                           | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                                     .QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED};
        mUboBuffer.CmdCopyToDevice(frameIndex, cmdBuffer, beforeAndAfter, beforeAndAfter);
    }
    bool ManagedUboBase::Exists() const
    {
        return mUboBuffer.Exists();
    }
    void ManagedUboBase::Destroy()
    {
        mUboBuffer.Destroy();
    }

}  // namespace foray