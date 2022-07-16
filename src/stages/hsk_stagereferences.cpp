#include "hsk_stagereferences.hpp"
#include "../utility/hsk_hash.hpp"

namespace hsk {
    uint64_t StageImageInfo::GetRequirementsHash() const
    {
        uint64_t hash = 0;
        AccumulateHash(hash, Size);
        if(Size == ESize::Custom)
        {
            AccumulateHash(hash, CustomSize.width);
            AccumulateHash(hash, CustomSize.height);
        }
        AccumulateHash(hash, Format);
        AccumulateHash(hash, UsageFlags);
        return hash;
    }
}  // namespace hsk