#pragma once
#include "../hsk_basics.hpp"

namespace hsk {
    template <typename T>
    inline void AccumulateHash(size_t& hash, const T& v)
    {
        // https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
        // https://www.boost.org/LICENSE_1_0.txt
        size_t vhash = std::hash<T>{}(v);
        hash ^= vhash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    /// @brief Calculates a hash value for any block of memory
    inline void AccumulateRaw(size_t& hash, const void* data, size_t size)
    {
        uint64_t        byteIndex = 0;
        const uint64_t* data64    = reinterpret_cast<const uint64_t*>(data);
        const uint8_t*  data8     = reinterpret_cast<const uint8_t*>(data);

        for(; byteIndex < size; byteIndex += 8)
        {
            AccumulateHash(hash, data64[byteIndex / 8]);
        }
        for(; byteIndex < size; byteIndex++)
        {
            AccumulateHash(hash, data8[byteIndex]);
        }
    }

}  // namespace hsk