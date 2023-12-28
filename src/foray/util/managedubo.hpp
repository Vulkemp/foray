#pragma once
#include "../basics.hpp"
#include "../glm.hpp"
#include "dualbuffer.hpp"

namespace foray::util {
    class ManagedUboBase : public core::ManagedResource
    {
      public:
        ManagedUboBase(core::Context* context, VkDeviceSize size, uint32_t stageBufferCount = INFLIGHT_FRAME_COUNT);
        virtual ~ManagedUboBase() = default;

        inline size_t SizeOfUbo() const { return mUboBuffer.GetDeviceBuffer().GetSize(); }

        FORAY_PROPERTY_R(UboBuffer)

        virtual void UpdateTo(uint32_t frameIndex) = 0;
        virtual void CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer);
        virtual void CmdPrepareForRead(VkCommandBuffer cmdBuffer, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask) const;
        virtual VkBufferMemoryBarrier2 MakeBarrierPrepareForRead(vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask) const;
        virtual bool Exists() const override;

        const core::ManagedBuffer* GetDeviceBuffer() const { return &(mUboBuffer.GetDeviceBuffer()); }

        operator vk::Buffer() const { return mUboBuffer.GetDeviceBuffer().GetBuffer(); }

        vk::DescriptorBufferInfo GetVkDescriptorBufferInfo() const;

        vk::Buffer GetVkBuffer() const { return mUboBuffer.GetDeviceBuffer().GetBuffer(); }

      protected:
        DualBuffer mUboBuffer;
    };

    /// @brief Template class for managing a UBO. For Host -> Device synchronization this utilises a DualBuffer
    /// @details
    /// # Usage
    ///  * GetData() / mData can be used like any normal object
    ///  * Every frame, call UpdateTo() to copy the entire UBO to the staging buffer, and CmdCopyToDevice to later copy from staging to device
    /// @tparam T_UBO 
    template <typename T_UBO>
    class ManagedUbo : public ManagedUboBase
    {
      protected:
        T_UBO mData = {};

      public:
        inline ManagedUbo(core::Context* context, std::string_view name);

        virtual ~ManagedUbo() = default;
        virtual void UpdateTo(uint32_t frameIndex) override;


        FORAY_PROPERTY_R(Data)
    };

    template <typename T_UBO>
    ManagedUbo<T_UBO>::ManagedUbo(core::Context* context, std::string_view name)
     : ManagedUboBase(context, sizeof(T_UBO))
    {
        if(name.size() > 0)
        {
            mUboBuffer.SetName(name);
        }
    }

    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::UpdateTo(uint32_t frameIndex)
    {
        mUboBuffer.StageFullBuffer(frameIndex, &mData);
    }
}  // namespace foray::util
