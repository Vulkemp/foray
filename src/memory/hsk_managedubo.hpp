#pragma once
#include "../hsk_basics.hpp"
#include "../hsk_glm.hpp"
#include "../memory/hsk_dualbuffer.hpp"
#include "../memory/hsk_managedbuffer.hpp"

namespace hsk {
    class ManagedUboBase : public DeviceResourceBase
    {
      public:
        inline size_t SizeOfUbo() const { return mUboBuffer.GetDeviceBuffer().GetSize(); }

        HSK_PROPERTY_ALLGET(UboBuffer)

        virtual void UpdateTo(uint32_t frameIndex) = 0;
        virtual void CmdCopyToDevice(uint32_t frameIndex, VkCommandBuffer cmdBuffer);
        virtual void Create(const VkContext* context, VkDeviceSize size, uint32_t stageBufferCount = INFLIGHT_FRAME_COUNT);
        virtual bool Exists() const override;
        virtual void Destroy() override;

      protected:
        DualBuffer mUboBuffer;
    };

    template <typename T_UBO>
    class ManagedUbo : public ManagedUboBase
    {
      protected:
        T_UBO mData;

      public:
        inline ManagedUbo();
        ~ManagedUbo();

        inline void Create(const VkContext* context, std::string_view name);
        virtual void        UpdateTo(uint32_t frameIndex) override;

        HSK_PROPERTY_ALL(Data)
    };

    template <typename T_UBO>
    ManagedUbo<T_UBO>::ManagedUbo() : ManagedUboBase(), mData{}
    {
    }

    template <typename T_UBO>
    void ManagedUbo<T_UBO>::Create(const VkContext* context, std::string_view name)
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
}  // namespace hsk
