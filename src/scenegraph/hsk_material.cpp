#include "hsk_material.hpp"

namespace hsk {
    void DefaultMaterial::WriteToBufferBlock(MaterialBufferBlock& block)
    {
        memcpy(&block, &Data, sizeof(Data));
    }
    const MaterialMeta& DefaultMaterial::GetMaterialMeta() const
    {
        return sMeta;
    }
}  // namespace hsk
