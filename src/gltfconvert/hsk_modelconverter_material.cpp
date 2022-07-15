#include "../scenegraph/globalcomponents/hsk_materialbuffer.hpp"
#include "hsk_modelconverter.hpp"

namespace hsk {

    inline int32_t _calcTextureIndex(int32_t gltfMatTexIndex, int32_t texOffset)
    {
        if(gltfMatTexIndex < 0)
        {
            return -1;
        }
        return gltfMatTexIndex + texOffset;
    }

    void ModelConverter::LoadMaterials()
    {
        for(int32_t i = 0; i < mGltfModel.materials.size(); i++)
        {
            const auto& gltfMaterial = mGltfModel.materials[i];
            auto&       material     = mMaterialBuffer.GetVector()[mIndexBindings.MaterialBufferOffset + i];

            // Pbr Base Info
            material.BaseColorFactor               = glm::vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0], gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
                                                               gltfMaterial.pbrMetallicRoughness.baseColorFactor[2], gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]);
            material.MetallicFactor                = gltfMaterial.pbrMetallicRoughness.metallicFactor;
            material.RoughnessFactor               = gltfMaterial.pbrMetallicRoughness.roughnessFactor;
            material.BaseColorTextureIndex         = _calcTextureIndex(gltfMaterial.pbrMetallicRoughness.baseColorTexture.index, mIndexBindings.TextureBufferOffset);
            material.MetallicRoughnessTextureIndex = _calcTextureIndex(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, mIndexBindings.TextureBufferOffset);

            // Aux Info
            material.EmissiveFactor       = glm::vec3(gltfMaterial.emissiveFactor[0], gltfMaterial.emissiveFactor[1], gltfMaterial.emissiveFactor[2]);
            material.EmissiveTextureIndex = _calcTextureIndex(gltfMaterial.emissiveTexture.index, mIndexBindings.TextureBufferOffset);
            material.NormalTextureIndex   = _calcTextureIndex(gltfMaterial.normalTexture.index, mIndexBindings.TextureBufferOffset);
        }
    }
}  // namespace hsk