#include "hsk_basesbt.hpp"

namespace hsk {
    ShaderBindingTableBase::ShaderBindingTableBase(VkDeviceSize entryDataSize) : mEntryDataSize(entryDataSize) {}

    void* ShaderBindingTableBase::GroupDataAt(int32_t index)
    {
        if(mEntryDataSize == 0)
        {
            return nullptr;
        }
        return reinterpret_cast<void*>(mGroupData.data() + (mEntryDataSize * index));
    }

    const void* ShaderBindingTableBase::GroupDataAt(int32_t index) const
    {
        if(mEntryDataSize == 0)
        {
            return nullptr;
        }
        return reinterpret_cast<const void*>(mGroupData.data() + (mEntryDataSize * index));
    }

    void ShaderBindingTableBase::SetData(int32_t index, const void* data)
    {
        if(mEntryDataSize == 0)
        {
            return;
        }
        size_t offset = index * mEntryDataSize;
        if(offset >= mGroupData.size())
        {
            mGroupData.resize((index + 1) * mEntryDataSize);
        }
        if(!!data)
        {
            memcpy(GroupDataAt(index), data, mEntryDataSize);
        }
    }

    void ShaderBindingTableBase::ArrayResized(size_t newSize)
    {
        if(mEntryDataSize == 0)
        {
            return;
        }
        size_t newBufferSize = newSize * mEntryDataSize;

        mGroupData.resize(newBufferSize);
    }

    ShaderBindingTableBase& ShaderBindingTableBase::SetEntryDataSize(VkDeviceSize newSize)
    {
        if(newSize == mEntryDataSize)
        {
            return *this;
        }
        size_t       groupCount = GetGroupArrayCount();
        VkDeviceSize oldSize    = mEntryDataSize;
        if(oldSize > 0 && groupCount > 0)
        {
            std::vector<uint8_t> oldDataCopy(mGroupData);
            mGroupData.clear();
            mGroupData.resize(newSize * groupCount);
            size_t entryCopySize = std::min(newSize, oldSize);
            for(int32_t i = 0; i < oldDataCopy.size(); i++)
            {
                const uint8_t* source = oldDataCopy.data() + (oldSize * i);
                uint8_t*       dest   = mGroupData.data() + (newSize * i);
                memcpy(dest, source, entryCopySize);
            }
        }
        mEntryDataSize = newSize;
        return *this;
    }

}  // namespace hsk
