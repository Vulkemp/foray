#include "foray_stringset.hpp"
#include <functional>

namespace foray::util {
    std::string_view StringSet::Add(const char* data, size_t size)
    {
        if(size > 0)
        {
            return Add(std::string_view(data, size));
        }
        else
        {
            return Add(std::string_view(data));
        }
    }

    std::string_view StringSet::Add(const std::string& str)
    {
        return Add(std::string_view(str));
    }

    std::string_view StringSet::Add(std::string_view str)
    {
        Assert(str.size() < BLOCKSIZE - 1, "Max string size exceeded");

        uint64_t hash = std::hash<std::string_view>()(str);

        const auto iter = mStringViews.find(hash);
        if(iter != mStringViews.cend())
        {
            return iter->second;
        }

        for(Heap<Block>& block : mBlocks)
        {
            if(block->CanAdd(str))
            {
                std::string_view stored = block->Add(str);
                mStringViews[hash]      = stored;
                return stored;
            }
        }
        mBlocks.emplace_back(Heap<Block>(Block()));
        std::string_view stored = mBlocks.back()->Add(str);
        mStringViews[hash]      = stored;
        return stored;
    }

    void StringSet::Clear()
    {
        mBlocks.clear();
    }

    size_t StringSet::GetCount() const
    {
        return mStringViews.size();
    }

    StringSet::~StringSet()
    {
    }


    bool StringSet::Block::CanAdd(std::string_view str) const
    {
        const uint8_t* End = Data + BLOCKSIZE;
        return End - str.size() > Free;
    }

    StringSet::Block::Block() : Data(), Free(Data) {}

    std::string_view StringSet::Block::Add(std::string_view str)
    {
        memcpy(Free, str.data(), str.size());
        std::string_view result(reinterpret_cast<char*>(Free), str.size());
        Free += str.size();
        return result;
    }
}  // namespace foray::util
