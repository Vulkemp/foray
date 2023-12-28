#include "stringset.hpp"
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
        mBlocks.emplace_back(Heap<Block>());
        mBlocks.back().New();
        std::string_view stored = mBlocks.back()->Add(str);
        mStringViews[hash]      = stored;
        return stored;
    }

    bool StringSet::Has(const char* data, size_t size) const
    {
        if(size > 0)
        {
            return Has(std::string_view(data, size));
        }
        else
        {
            return Has(std::string_view(data));
        }
    }

    bool StringSet::Has(const std::string& str) const
    {
        return Has(std::string_view(str));
    }

    bool StringSet::Has(std::string_view str) const
    {
        uint64_t hash = std::hash<std::string_view>()(str);
        return mStringViews.contains(hash);
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
        size_t Capacity = End - Free;
        return str.size() + 1 <= Capacity;
    }

    StringSet::Block::Block() : Data(), Free(Data) {}

    std::string_view StringSet::Block::Add(std::string_view str)
    {
        memcpy(Free, str.data(), str.size());
        std::string_view result(reinterpret_cast<char*>(Free), str.size());
        Free += str.size() + 1;
        return result;
    }
}  // namespace foray::util
