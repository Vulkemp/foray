#pragma once
#include "../core/managedbuffer.hpp"
#include "../basics.hpp"
#include "../mem.hpp"

namespace foray::util {

    /// @brief Helper class for organizing CPU -> GPU data synchronisation. In flight data is stored on host side, and copied to GPU right before being used.
    /// @details
    /// Temporarily stores CPU-calculated states in staging buffers (1x per frame in flight)
    /// The GPU can retrieve the CPU-side data as needed via a copy command
    /// # Uploading Data
    ///  * Stage sections or full buffer
    ///  * Record CmdCopyToDevice
    /// # Reading data
    ///  * Record CmdPrepareForRead
    ///  * Access the buffer as you wish
    class DualBuffer : public NoMoveDefaults
    {
      public:
        /// @brief Creates the dualbuffer setup
        /// @param devicebufferCreateInfo Used to initate the drawside buffer. Same size is used for staging buffers
        DualBuffer(core::Context* context, const core::ManagedBuffer::CreateInfo& devicebufferCreateInfo, uint32_t stageBufferCount = INFLIGHT_FRAME_COUNT);

        /// @brief Updates the entire staging buffer
        virtual void StageFullBuffer(uint32_t frameIndex, const void* data);

        /// @brief Updates a section of a staging buffer
        virtual void StageSection(uint32_t frameIndex, const void* data, size_t destOffset, size_t size);

        /// @brief Writes commands required to copy the changes recorded in the indexed staging buffer to the device buffer
        /// @remark This will mirror any buffer copies submitted via 'StageSection'/'StageFullBuffer' calls before
        virtual void CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer);

        /// @brief Prepares the device buffer for read access with a pipeline barrier (must be called before read access happens to the buffer)
        virtual void CmdPrepareForRead(VkCommandBuffer cmdBuffer, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask) const;

        /// @brief Returns a barrier which prepares the device buffer for read access
        virtual VkBufferMemoryBarrier2 MakeBarrierPrepareForRead(vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask) const;

        inline bool Exists() const { return mDeviceBuffer.Exists(); }

        inline std::string GetName() const { return mDeviceBuffer.GetName(); }
        DualBuffer&        SetName(std::string_view name);

        inline vk::DescriptorBufferInfo GetVkDescriptorInfo() const { return vk::DescriptorBufferInfo{.buffer = mDeviceBuffer.GetBuffer(), .offset = 0U, .range = VK_WHOLE_SIZE}; }
        inline vk::Buffer GetDeviceVkBuffer() const { return mDeviceBuffer.GetBuffer(); }

        virtual ~DualBuffer() = default;

        FORAY_GETTER_CR(DeviceBuffer)

      protected:
        /// @brief Memory locations the permanently mapped staging buffers are mapped to
        std::vector<void*> mStagingBufferMaps;
        /// @brief Permanently mapped host-local staging buffers (one per frame in flight)
        std::vector<Heap<core::ManagedBuffer>> mStagingBuffers;
        /// @brief Record of all buffer copies submitted. Added to when writing to staging buffers, cleared when building the commandbuffer
        std::vector<std::vector<VkBufferCopy>> mBufferCopies;
        /// @brief The buffer used by the device
        core::ManagedBuffer mDeviceBuffer;
    };
}  // namespace foray::util