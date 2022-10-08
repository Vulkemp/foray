#pragma once
#include "../core/foray_managedbuffer.hpp"
#include <optional>

namespace foray::util {
    struct BufferSection
    {
        VkDeviceSize offset = 0;
        VkDeviceSize count  = 0;

        inline BufferSection Merge(const BufferSection& other)
        {
            VkDeviceSize start = std::min(offset, other.offset);
            VkDeviceSize end   = std::max(offset + count, other.offset + other.count);
            return BufferSection{.offset = start, .count = end - start};
        }
    };

    template <typename TClass>
    class ManagedVectorBuffer : public core::DeviceResourceBase
    {
      public:
        const bool DeviceLocal;

        inline ManagedVectorBuffer() : DeviceLocal(false) {}
        inline explicit ManagedVectorBuffer(const core::VkContext* context) : DeviceLocal(false), mContext(context) {}
        inline explicit ManagedVectorBuffer(const core::VkContext* context, bool deviceLocal) : DeviceLocal(deviceLocal), mContext(context) {}

        void         InitOrUpdate(std::optional<BufferSection> section = {});
        virtual void Destroy() override;
        virtual bool Exists() const override { return mBuffer.Exists(); }

        inline virtual std::string_view   GetName() const { return mBuffer.GetName(); }
        inline virtual ManagedVectorBuffer<TClass>& SetName(std::string_view name) override;

        inline virtual ~ManagedVectorBuffer() { Destroy(); }

        FORAY_PROPERTY_ALL(Context)
        FORAY_PROPERTY_GET(Vector)
        FORAY_PROPERTY_CGET(Vector)
        FORAY_PROPERTY_GET(Buffer)
        FORAY_PROPERTY_CGET(Buffer)
        FORAY_PROPERTY_CGET(DeviceCount)
        FORAY_PROPERTY_CGET(DeviceCapacity)

        inline VkDeviceSize GetDeviceSize() { return mDeviceCapacity * sizeof(TClass); }

      protected:
        inline void CreateBuffer(VkDeviceSize capacity);
        inline void UploadToBuffer(BufferSection section);

        const core::VkContext*    mContext        = nullptr;
        std::vector<TClass> mVector         = {};
        core::ManagedBuffer       mBuffer         = {};
        VkDeviceSize        mDeviceCount    = 0;
        VkDeviceSize        mDeviceCapacity = 0;
        void*               mHostMemoryMap  = nullptr;
    };

    template <typename TClass>
    void ManagedVectorBuffer<TClass>::InitOrUpdate(std::optional<BufferSection> section)
    {
        if(section.has_value())
        {
            FORAY_ASSERTFMT(section.value().offset + section.value().count <= mVector.size(),
                          "Buffer Update: BufferSection(offset = {}, count = {}) indicates vector read out of bounds (vector.size() = {})!", section.value().offset,
                          section.value().count, mVector.size())
        }

        BufferSection updateSection = section.has_value() ? section.value() : BufferSection{.offset = 0, .count = mVector.size()};

        VkDeviceSize totalCountInVector = (VkDeviceSize)mVector.size();

        if(totalCountInVector > mDeviceCapacity)
        {
            Destroy();
            CreateBuffer(totalCountInVector + (totalCountInVector / 4));

            updateSection = BufferSection{.offset = 0, .count = totalCountInVector};
        }
        UploadToBuffer(updateSection);
        mDeviceCount = totalCountInVector;
    }

    template <typename TClass>
    void ManagedVectorBuffer<TClass>::CreateBuffer(VkDeviceSize capacity)
    {
        mDeviceCapacity = capacity;

        VkDeviceSize bufferSize = mDeviceCapacity * sizeof(TClass);

        mBuffer.Create(mContext, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize,
                       DeviceLocal ? VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE : VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
                       DeviceLocal ? 0 : VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        if(!DeviceLocal)
        {
            mBuffer.Map(mHostMemoryMap);
        }
    }
    template <typename TClass>
    void ManagedVectorBuffer<TClass>::UploadToBuffer(BufferSection section)
    {
        TClass* uploadData = mVector.data() + section.offset;

        if(DeviceLocal)
        {
            VkDeviceSize deviceSize   = section.count * sizeof(TClass);
            VkDeviceSize deviceOffset = section.offset * sizeof(TClass);
            mBuffer.WriteDataDeviceLocal(uploadData, deviceSize, deviceOffset);
        }
        else
        {
            size_t memorySize   = section.count * sizeof(TClass);
            size_t memoryOffset = section.offset * sizeof(TClass);

            uint8_t* memoryDestination = reinterpret_cast<uint8_t*>(mHostMemoryMap) + memoryOffset;

            memcpy(reinterpret_cast<void*>(memoryDestination), uploadData, memorySize);
        }
    }
    template <typename TClass>
    void ManagedVectorBuffer<TClass>::Destroy()
    {
        if(!DeviceLocal && mBuffer.GetIsMapped())
        {
            mBuffer.Unmap();
        }
        mBuffer.Destroy();
    }

    template <typename TClass>
    inline ManagedVectorBuffer<TClass>& ManagedVectorBuffer<TClass>::SetName(std::string_view name)
    {
        mBuffer.SetName(name);
        return *this;
    }

}  // namespace foray