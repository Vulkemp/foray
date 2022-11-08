#pragma once
#include "../foray_basics.hpp"
#include "../core/foray_managedbuffer.hpp"

namespace foray::util {

    /// @brief Helper class for organizing CPU -> GPU data synchronisation. In flight data is stored on host side, and copied to GPU right before being used.
    /// @remark Temporarily stores CPU-calculated states in staging buffers (1x per frame in flight)
    /// @remark The GPU can retrieve the CPU-side data as needed via a copy command
    class DualBuffer : public NoMoveDefaults
    {
      public:
        /// @brief Creates the dualbuffer setup
        /// @param devicebufferCreateInfo Used to initate the drawside buffer. Same size is used for staging buffers
        void Create(core::Context* context, const core::ManagedBuffer::CreateInfo& devicebufferCreateInfo, uint32_t stageBufferCount = INFLIGHT_FRAME_COUNT);

        /// @brief Updates the entire staging buffer
        void StageFullBuffer(uint32_t frameIndex, const void* data);

        /// @brief Updates a section of a staging buffer
        void StageSection(uint32_t frameIndex, const void* data, size_t destOffset, size_t size);

        /// @brief State context for the buffer before/after copy operation
        struct DeviceBufferState
        {
            /// @brief Access flags determining mask of memory accesses
            VkAccessFlags AccessFlags = 0;
            /// @brief Pipeline stage flags determining access types
            VkPipelineStageFlags PipelineStageFlags = 0;
            /// @brief Queue using the buffer
            uint32_t QueueFamilyIndex = 0;

            bool operator==(const DeviceBufferState& other) const;
        };

        static constexpr uint32_t TRANSFER_QUEUE_AUTO = ~0U;

        /// @brief Writes commands required to copy the changes recorded in the indexed staging buffer to the device buffer
        /// @remark This will mirror any buffer copies submitted via 'StageSection'/'StageFullBuffer' calls before
        /// @param before Declare the usage of the buffer before copy. Used to configure pipeline barriers
        /// @param after Declare the usage of the buffer after copy. Used to configure pipeline barriers
        /// @param transferQueueFamilyIndex Queue that is used to copy (same queue that cmdBuffer is submitted to). If set to 'TRANSFER_QUEUE_AUTO' / omitted, uses the queue specified in 'before' argument
        void CmdCopyToDevice(uint32_t                 frameIndex,
                             VkCommandBuffer          cmdBuffer,
                             const DeviceBufferState& before,
                             const DeviceBufferState& after,
                             uint32_t                 transferQueueFamilyIndex = TRANSFER_QUEUE_AUTO);

        inline bool Exists() const { return mDeviceBuffer.Exists(); }

        inline std::string GetName() const { return mDeviceBuffer.GetName(); }
        DualBuffer& SetName(std::string_view name);

        void Destroy();

        inline virtual ~DualBuffer() { Destroy(); }

        FORAY_GETTER_CR(DeviceBuffer)

      protected:
        /// @brief Memory locations the permanently mapped staging buffers are mapped to
        std::vector<void*> mStagingBufferMaps;
        /// @brief Permanently mapped host-local staging buffers (one per frame in flight)
        std::vector<std::unique_ptr<core::ManagedBuffer>> mStagingBuffers;
        /// @brief Record of all buffer copies submitted. Added to when writing to staging buffers, cleared when building the commandbuffer
        std::vector<std::vector<VkBufferCopy>> mBufferCopies;
        /// @brief The buffer used by the device
        core::ManagedBuffer mDeviceBuffer;
    };
}  // namespace foray