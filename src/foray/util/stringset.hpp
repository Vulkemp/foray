#pragma once
#include "../basics.hpp"
#include "../mem.hpp"
#include <unordered_map>
#include <vector>

namespace foray::util {
    class StringSet
    {
      public:
        using StringViewMap = std::unordered_map<uint64_t, std::string_view>;
        struct iterator : private StringViewMap::iterator
        {
            using Base = StringViewMap::iterator;

          public:
            iterator() : Base() {}
            iterator(Base&& base) : Base(base) {}
            iterator& operator++()
            {
                Base::operator++();
                return *this;
            }
            iterator          operator++(int v) { return Base::operator++(v); }
            bool              operator==(iterator other) const { return ((Base) * this) == (Base)other; }
            bool              operator!=(iterator other) const { return !(*this == other); }
            std::string_view& operator*() const { return Base::operator*().second; }
        };
        struct const_iterator : private StringViewMap::const_iterator
        {
            using Base = StringViewMap::const_iterator;

          public:
            const_iterator() : Base() {}
            const_iterator(Base&& base) : Base(base) {}
            const_iterator& operator++()
            {
                Base::operator++();
                return *this;
            }
            const_iterator          operator++(int v) { return Base::operator++(v); }
            bool                    operator==(const_iterator other) const { return ((Base) * this) == (Base)other; }
            bool                    operator!=(const_iterator other) const { return !(*this == other); }
            const std::string_view& operator*() const { return Base::operator*().second; }
        };

        std::string_view Add(const char* data, size_t size = 0);
        std::string_view Add(const std::string& str);
        std::string_view Add(std::string_view str);

        template <typename TIter>
        void AddRange(TIter begin, const TIter end)
        {
            for(; begin != end; begin++)
            {
                Add(*begin);
            }
        }

        bool Has(const char* data, size_t size = 0) const;
        bool Has(const std::string& str) const;
        bool Has(std::string_view str) const;

        inline iterator       begin() { return iterator(mStringViews.begin()); }
        inline iterator       end() { return iterator(mStringViews.end()); }
        inline const_iterator cbegin() { return const_iterator(mStringViews.cbegin()); }
        inline const_iterator cend() { return const_iterator(mStringViews.cend()); }
        inline const_iterator begin() const { return const_iterator(mStringViews.cbegin()); }
        inline const_iterator end() const { return const_iterator(mStringViews.cend()); }

        void Clear();

        size_t GetCount() const;

        StringSet() = default;
        template <typename TIter>
        StringSet(TIter begin, const TIter end)
        {
            AddRange(begin, end);
        }
        StringSet(const StringSet& other)            = delete;
        StringSet(StringSet&& other)                 = delete;
        StringSet& operator=(const StringSet& other) = delete;
        virtual ~StringSet();

        static const size_t BLOCKSIZE  = 4096 - sizeof(uint8_t*);
        static const size_t BLOCKALIGN = 4096;

      protected:
        struct alignas(BLOCKALIGN) Block
        {
            uint8_t  Data[BLOCKSIZE];
            uint8_t* Free;

            Block();

            bool             CanAdd(std::string_view len) const;
            std::string_view Add(std::string_view str);
        };

        std::vector<Heap<Block>> mBlocks;
        StringViewMap            mStringViews;
    };
}  // namespace foray::util
