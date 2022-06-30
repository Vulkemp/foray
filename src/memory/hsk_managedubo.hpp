#pragma once
#include "../hsk_basics.hpp"
#include "../hsk_glm.hpp"
#include "../memory/hsk_managedbuffer.hpp"

namespace hsk {
    class UboInterface : public DeviceResourceBase
    {
      public:
        virtual void   Init(const VkContext* context, bool update = false) = 0;
        virtual void   Update()                                            = 0;
        virtual size_t SizeOfUbo() const                                   = 0;
        virtual void*  UboData()                                           = 0;

        // virtual void                 BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const = 0;
        // virtual VkWriteDescriptorSet BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const                             = 0;

        template <typename T_UBO>
        T_UBO& getUBO()
        {
            assert(sizeof(T_UBO) == SizeOfUbo());
            T_UBO* data = reinterpret_cast<T_UBO*>(UboData());
            return *data;
        }
    };

    template <typename T_UBO>
    class ManagedUbo : public UboInterface
    {
      protected:
        ManagedBuffer mManagedBuffer = {};
        T_UBO         mUbo           = {};
        void*         mMapped        = nullptr;
        const bool    mMapPersistent = false;

      public:
        using Ptr = std::shared_ptr<ManagedUbo<T_UBO>>;

        explicit inline ManagedUbo(bool mapPersistent = false);
        ~ManagedUbo();

        HSK_PROPERTY_ALL(Ubo)
        HSK_PROPERTY_GET(ManagedBuffer)
        HSK_PROPERTY_CGET(ManagedBuffer)
        HSK_PROPERTY_CGET(MapPersistent)

        inline virtual void   Init(const VkContext* context, bool update = false) override;
        inline virtual void   Update() override;
        inline virtual void   Cleanup() override;
        inline virtual bool Exists() const override {return mManagedBuffer.Exists();}
        inline virtual size_t SizeOfUbo() const override;
        inline virtual void*  UboData() override { return &mUbo; }

        inline virtual std::string_view   GetName() const { return mManagedBuffer.GetName(); }
        inline virtual ManagedUbo<T_UBO>& SetName(std::string_view name);
    };

    template <typename T_UBO>
    ManagedUbo<T_UBO>::ManagedUbo(bool mapPersistent) : mManagedBuffer(), mUbo(), mMapPersistent(mapPersistent)
    {
        mName = "Unnamed Ubo";
    }

    template <typename T_UBO>
    inline ManagedUbo<T_UBO>::~ManagedUbo()
    {
        Cleanup();
    }

    template <typename T_UBO>
    void ManagedUbo<T_UBO>::Init(const VkContext* context, bool update)
    {

        ManagedBuffer::ManagedBufferCreateInfo bufferCI;

        bufferCI.AllocationCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        bufferCI.AllocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        bufferCI.BufferCreateInfo.size        = sizeof(T_UBO);
        bufferCI.BufferCreateInfo.usage       = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferCI.BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


        mManagedBuffer.Create(context, bufferCI);

        if(mMapPersistent)
        {
            mManagedBuffer.Map(mMapped);
        }
        if(update)
        {
            Update();
        }
    }

    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::Update()
    {
        if(!mMapPersistent)
        {
            mManagedBuffer.Map(mMapped);
        }
        memcpy(mMapped, &mUbo, sizeof(mUbo));
        if(!mMapPersistent)
        {
            mManagedBuffer.Unmap();
        }
    }

    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::Cleanup()
    {
        if(mManagedBuffer.GetAllocation() && mMapPersistent)
        {
            mManagedBuffer.Unmap();
        }
        mManagedBuffer.Cleanup();
    }
    template <typename T_UBO>
    inline size_t ManagedUbo<T_UBO>::SizeOfUbo() const
    {
        return sizeof(T_UBO);
    }

    template <typename T_UBO>
    inline ManagedUbo<T_UBO>& ManagedUbo<T_UBO>::SetName(std::string_view name)
    {
        mManagedBuffer.SetName(name);
        return *this;
    }

}  // namespace hsk
