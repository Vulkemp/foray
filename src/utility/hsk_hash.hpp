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


}  // namespace hsk