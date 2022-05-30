#pragma once
#include "../hsk_basics.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include <glm/glm.hpp>

namespace hsk {
    class UboInterface
    {
      public:
        virtual void   Init(const VkContext* context, bool update = false) = 0;
        virtual void   Update()                                      = 0;
        virtual void   Cleanup()                                     = 0;
        virtual size_t SizeOfUbo() const                             = 0;
        virtual void*  UboData()                                     = 0;

        virtual void                 BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const = 0;
        virtual VkWriteDescriptorSet BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const                             = 0;

        template <typename T_UBO>
        T_UBO& getUBO()
        {
            assert(sizeof(T_UBO) == SizeOfUbo());
            T_UBO* data = reinterpret_cast<T_UBO*>(UboData());
            return *data;
        }
    };

    template <typename T_UBO>
    class ManagedUbo : public UboInterface, public NoMoveDefaults
    {
      protected:
        ManagedBuffer mBuffer        = {};
        T_UBO         mUbo           = {};
        void*         mMapped        = nullptr;
        const bool    mMapPersistent = false;

      public:
        using Ptr = std::shared_ptr<ManagedUbo<T_UBO>>;

        inline ManagedUbo();
        explicit inline ManagedUbo(bool mapPersistent = false);
        ~ManagedUbo();

        HSK_PROPERTY_ALL(Ubo)
        HSK_PROPERTY_GET(Buffer)
        HSK_PROPERTY_CGET(Buffer)
        HSK_PROPERTY_CGET(MapPersistent)

        inline virtual void   Init(const VkContext* context, bool update = false) override;
        inline virtual void   Update() override;
        inline virtual void   Cleanup() override;
        inline virtual size_t SizeOfUbo() const override;
        inline virtual void*  UboData() override { return &mUbo; }

        inline virtual void                 BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const override;
        inline virtual VkWriteDescriptorSet BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const override;
    };

    template <typename T_UBO>
    ManagedUbo<T_UBO>::ManagedUbo(bool mapPersistent) : mBuffer(), mUbo(), mMapPersistent(mapPersistent)
    {
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

        bufferCI.AllocationCreateInfo.usage                   = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        bufferCI.AllocationCreateInfo.flags                   = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        bufferCI.BufferCreateInfo.size               = sizeof(T_UBO);
        bufferCI.BufferCreateInfo.usage              = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferCI.BufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;


        mBuffer.Create(context, bufferCI);

        if(update)
        {
            Update();
        }
        if(mMapPersistent)
        {
            mBuffer.Map(mMapped);
        }
    }

    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::Update()
    {
        if(!mMapPersistent)
        {
            mBuffer.Map(mMapped);
        }
        memcpy(mMapped, &mUbo, sizeof(mUbo));
        if(!mMapPersistent)
        {
            mBuffer.Unmap();
        }
    }

    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::Cleanup()
    {
        if(mBuffer.GetAllocation() && mMapPersistent)
        {
            mBuffer.Unmap();
        }
        mBuffer.Destroy();
    }
    template <typename T_UBO>
    inline size_t ManagedUbo<T_UBO>::SizeOfUbo() const
    {
        return sizeof(T_UBO);
    }
    template <typename T_UBO>
    inline void ManagedUbo<T_UBO>::BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding, VkWriteDescriptorSet& dest) const
    {
        dest.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dest.dstSet          = descriptorSet;
        dest.descriptorType  = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dest.dstBinding      = binding;
        dest.pBufferInfo     = &mBuffer.GetDescriptorInfo();
        dest.descriptorCount = 1;
    }
    template <typename T_UBO>
    inline VkWriteDescriptorSet ManagedUbo<T_UBO>::BuildWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const
    {
        VkWriteDescriptorSet result{};
        BuildWriteDescriptorSet(descriptorSet, binding, result);
        return result;
    }
}  // namespace hsk
