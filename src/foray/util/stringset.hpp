#pragma once
#include "../basics.hpp"
#include "../mem.hpp"
#include <unordered_map>
#include <vector>

namespace foray::util {
    class StringSet
    {
      public:
        std::string_view Add(const char* data, size_t size = 0);
        std::string_view Add(const std::string& str);
        std::string_view Add(std::string_view str);

        void Clear();

        size_t GetCount() const;

        StringSet() = default;
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

            bool CanAdd(std::string_view len) const;
            std::string_view Add(std::string_view str);
        };

        std::vector<Heap<Block>>                            mBlocks;
        std::unordered_map<uint64_t, std::string_view> mStringViews;
    };
}  // namespace foray::util
