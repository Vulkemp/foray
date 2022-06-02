#pragma once
#include "hsk_managedbuffer.hpp"
#include <optional>

namespace hsk {
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
    class ManagedVectorBuffer : public NoMoveDefaults
    {
      public:
        inline ManagedVectorBuffer() {}
        inline explicit ManagedVectorBuffer(const VkContext* context) : mContext(context) {}

        void InitOrUpdate(std::optional<BufferSection> section = {});
        void Cleanup();

        inline virtual ~ManagedVectorBuffer() { Cleanup(); }

        HSK_PROPERTY_ALL(Context)
        HSK_PROPERTY_GET(Vector)
        HSK_PROPERTY_CGET(Vector)
        HSK_PROPERTY_GET(DeviceBuffer)
        HSK_PROPERTY_CGET(DeviceBuffer)
        HSK_PROPERTY_CGET(DeviceCount)
        HSK_PROPERTY_CGET(DeviceCapacity)

        inline VkDeviceSize GetDeviceSize() { return mDeviceCapacity * sizeof(TClass); }

      protected:
        inline void CreateBuffer(VkDeviceSize capacity);
        inline void UploadToBuffer(BufferSection section);

        const VkContext*    mContext        = nullptr;
        std::vector<TClass> mVector         = {};
        ManagedBuffer       mDeviceBuffer   = {};
        VkDeviceSize        mDeviceCount    = 0;
        VkDeviceSize        mDeviceCapacity = 0;
    };

    template <typename TClass>
    void ManagedVectorBuffer<TClass>::InitOrUpdate(std::optional<BufferSection> section)
    {
        if(section.has_value())
        {
            HSK_ASSERTFMT(section.value().offset + section.value().count <= mVector.size(),
                          "Buffer Update: BufferSection(offset = {}, count = {}) indicates vector read out of bounds (vector.size() = {})!", section.value().offset,
                          section.value().count, mVector.size())
        }

        BufferSection updateSection = section.has_value() ? section.value() : BufferSection{.offset = 0, .count = mVector.size()};

        VkDeviceSize totalCountInVector = (VkDeviceSize)mVector.size();

        if(totalCountInVector > mDeviceCapacity)
        {
            Cleanup();
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

        mDeviceBuffer.Create(mContext, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize,
                             VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    }
    template <typename TClass>
    void ManagedVectorBuffer<TClass>::UploadToBuffer(BufferSection section)
    {
        TClass*      uploadData   = mVector.data() + section.offset;
        VkDeviceSize deviceSize   = section.count * sizeof(TClass);
        VkDeviceSize deviceOffset = section.offset * sizeof(TClass);

        mDeviceBuffer.WriteDataDeviceLocal(uploadData, deviceSize, deviceOffset);
    }
    template <typename TClass>
    void ManagedVectorBuffer<TClass>::Cleanup()
    {
        mDeviceBuffer.Destroy();
    }
}  // namespace hsk