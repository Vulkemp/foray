#pragma once
#include "../foray_basics.hpp"
#include "../foray_glm.hpp"
#include "foray_dualbuffer.hpp"

namespace foray::util {
    class ManagedUboBase : public core::ManagedResource
    {
      public:
        inline size_t SizeOfUbo() const { return mUboBuffer.GetDeviceBuffer().GetSize(); }

        FORAY_PROPERTY_R(UboBuffer)

        virtual void UpdateTo(uint32_t frameIndex) = 0;
        virtual void CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer);
        virtual void Create(core::Context* context, VkDeviceSize size, uint32_t stageBufferCount = INFLIGHT_FRAME_COUNT);
        virtual bool Exists() const override;
        virtual void Destroy() override;

        const core::ManagedBuffer* GetDeviceBuffer() const { return &(mUboBuffer.GetDeviceBuffer()); }

        operator VkBuffer() const { return mUboBuffer.GetDeviceBuffer().GetBuffer(); }

        VkDescriptorBufferInfo GetVkDescriptorBufferInfo() const;

        VkBuffer GetVkBuffer() const { return mUboBuffer.GetDeviceBuffer().GetBuffer(); }

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
        T_UBO mData;

      public:
        inline ManagedUbo();
        ~ManagedUbo();

        inline void  Create(core::Context* context, std::string_view name);
        virtual void UpdateTo(uint32_t frameIndex) override;


        FORAY_PROPERTY_R(Data)
    };

    template <typename T_UBO>
    ManagedUbo<T_UBO>::ManagedUbo() : ManagedUboBase(), mData{}
    {
    }

    template <typename T_UBO>
    void ManagedUbo<T_UBO>::Create(core::Context* context, std::string_view name)
    {
        ManagedUboBase::Create(context, sizeof(T_UBO));
        if(name.size() > 0)
        {
            mUboBuffer.SetName(name);
        }
    }

    template <typename T_UBO>
    inline ManagedUbo<T_UBO>::~ManagedUbo()
    {
        ManagedUboBase::Destroy();
    }

    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::UpdateTo(uint32_t frameIndex)
    {
        mUboBuffer.StageFullBuffer(frameIndex, &mData);
    }
}  // namespace foray::util
