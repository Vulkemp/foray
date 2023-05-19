#pragma once
#include "../core/managedbuffer.hpp"
#include "../mem.hpp"
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

    /// @brief Class maintaining a resizable array of templated classes in a managed buffer
    template <typename TClass>
    class ManagedVectorBuffer : public core::ManagedResource
    {
      public:
        const bool DeviceLocal;

        inline ManagedVectorBuffer() : DeviceLocal(false) {}
        inline explicit ManagedVectorBuffer(core::Context* context) : DeviceLocal(false), mContext(context) {}
        inline explicit ManagedVectorBuffer(core::Context* context, bool deviceLocal) : DeviceLocal(deviceLocal), mContext(context) {}

        void         InitOrUpdate(std::optional<BufferSection> section = {});
        virtual bool Exists() const override { return mBuffer.Exists(); }

        inline virtual std::string GetName() const { return mBuffer->GetName(); }
        inline virtual void        SetName(std::string_view name) override;

        virtual ~ManagedVectorBuffer();

        FORAY_PROPERTY_V(Context)
        FORAY_GETTER_MR(Vector)
        FORAY_GETTER_CR(Vector)
        FORAY_GETTER_MR(Buffer)
        FORAY_GETTER_CR(Buffer)
        FORAY_GETTER_V(DeviceCount)
        FORAY_GETTER_V(DeviceCapacity)

        inline VkDeviceSize GetDeviceSize() { return mDeviceCapacity * sizeof(TClass); }

        void Clear();

      protected:
        inline void CreateBuffer(VkDeviceSize capacity);
        inline void UploadToBuffer(BufferSection section);

        core::Context*             mContext        = nullptr;
        std::vector<TClass>        mVector         = {};
        Local<core::ManagedBuffer> mBuffer         = {};
        VkDeviceSize               mDeviceCount    = 0;
        VkDeviceSize               mDeviceCapacity = 0;
        void*                      mHostMemoryMap  = nullptr;
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

        mBuffer.New(mContext, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize,
                    DeviceLocal ? VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE : VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
                    DeviceLocal ? 0 : VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        if(!DeviceLocal)
        {
            mBuffer->Map(mHostMemoryMap);
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
            mBuffer->WriteDataDeviceLocal(uploadData, deviceSize, deviceOffset);
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
    ManagedVectorBuffer<TClass>::~ManagedVectorBuffer()
    {
        if(!DeviceLocal && mBuffer && mBuffer->GetIsMapped())
        {
            mBuffer->Unmap();
        }
    }

    template <typename TClass>
    void ManagedVectorBuffer<TClass>::Clear()
    {
        mVector.clear();
        if(!DeviceLocal && mBuffer && mBuffer->GetIsMapped())
        {
            mBuffer->Unmap();
        }
        mBuffer         = nullptr;
        mDeviceCount    = 0;
        mDeviceCapacity = 0;
    }

    template <typename TClass>
    inline void ManagedVectorBuffer<TClass>::SetName(std::string_view name)
    {
        mBuffer->SetName(name);
    }

}  // namespace foray::util