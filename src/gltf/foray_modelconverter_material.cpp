#include "../scene/globalcomponents/foray_materialmanager.hpp"
#include "foray_modelconverter.hpp"

namespace foray::gltf {

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
        const std::string extIor     = "KHR_materials_ior";
        const std::string extIor_ior = "ior";

        const std::string extTransmission                           = "KHR_materials_transmission";
        const std::string extTransmission_transmissionFactor        = "transmissionFactor";
        const std::string extTransmission_transmissionTexture       = "transmissionTexture";
        const std::string extTransmission_transmissionTexture_index = "index";

        const std::string extVolume                     = "KHR_materials_volume";
        const std::string extVolume_attenuationDistance = "attenuationDistance";
        const std::string extVolume_attenuationColor    = "attenuationColor";

        const std::string extEmissiveStrength           = "KHR_materials_emissive_strength";
        const std::string extEmissiveStrength_value     = "emissiveStrength";


        for(int32_t i = 0; i < (int32_t)mGltfModel.materials.size(); i++)
        {
            const auto& gltfMaterial = mGltfModel.materials[i];
            auto&       material     = mMaterialBuffer.GetVector()[mIndexBindings.MaterialBufferOffset + i];

            material.Flags |= (gltfMaterial.alphaMode == "OPAQUE" ? (scene::MaterialFlags)scene::MaterialFlagBits::FullyOpaque : 0);
            material.Flags |= (gltfMaterial.doubleSided ? (scene::MaterialFlags)scene::MaterialFlagBits::DoubleSided : 0);

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

            {  // EmissiveStrength
                const auto iter = gltfMaterial.extensions.find(extEmissiveStrength);
                if(iter != gltfMaterial.extensions.cend())
                {
                    if(iter->second.IsObject() && iter->second.Has(extEmissiveStrength_value))
                    {
                        const auto value = iter->second.Get(extEmissiveStrength_value);
                        if(value.IsNumber())
                        {
                            material.EmissiveFactor *= (fp32_t)value.GetNumberAsDouble();
                        }
                    }
                }
            }

            {  // Index of Refraction
                const auto iter = gltfMaterial.extensions.find(extIor);
                if(iter != gltfMaterial.extensions.cend())
                {
                    if(iter->second.IsObject() && iter->second.Has(extIor_ior))
                    {
                        const auto value = iter->second.Get(extIor_ior);
                        if(value.IsNumber())
                        {
                            material.IndexOfRefraction = (fp32_t)value.GetNumberAsDouble();
                        }
                    }
                }
            }
            {  // Transmission
                const auto iter = gltfMaterial.extensions.find(extTransmission);
                if(iter != gltfMaterial.extensions.cend())
                {
                    if(iter->second.IsObject())
                    {
                        if(iter->second.Has(extTransmission_transmissionFactor))
                        {
                            const tinygltf::Value factor = iter->second.Get(extTransmission_transmissionFactor);
                            if(factor.IsNumber())
                            {
                                material.TransmissionFactor = (fp32_t)factor.GetNumberAsDouble();
                            }
                        }

                        if(iter->second.Has(extTransmission_transmissionTexture))
                        {
                            const tinygltf::Value texInfoObj = iter->second.Get(extTransmission_transmissionTexture);
                            if(texInfoObj.IsObject() && texInfoObj.Has(extTransmission_transmissionTexture_index))
                            {
                                const tinygltf::Value index = texInfoObj.Get(extTransmission_transmissionTexture_index);
                                if(index.IsNumber())
                                {
                                    material.TransmissionTextureIndex = _calcTextureIndex((int32_t)index.GetNumberAsInt(), mIndexBindings.TextureBufferOffset);
                                }
                            }
                        }
                    }
                }
            }
            {  // Volume
                const auto iter = gltfMaterial.extensions.find(extVolume);
                if(iter != gltfMaterial.extensions.cend())
                {
                    if(iter->second.IsObject())
                    {
                        if(iter->second.Has(extVolume_attenuationColor))
                        {
                            const tinygltf::Value valueArray = iter->second.Get(extVolume_attenuationColor);
                            if(valueArray.IsArray())
                            {
                                for(int32_t i = 0; i < (int32_t)valueArray.ArrayLen() && i < 3; i++)
                                {
                                    const tinygltf::Value value = valueArray.Get(i);
                                    if(value.IsNumber())
                                    {
                                        material.AttenuationColor[i] = (fp32_t)value.GetNumberAsDouble();
                                    }
                                }
                            }
                        }

                        if(iter->second.Has(extVolume_attenuationDistance))
                        {
                            const tinygltf::Value distance = iter->second.Get(extVolume_attenuationDistance);
                            if(distance.IsNumber())
                            {
                                material.AttenuationDistance = (fp32_t)distance.GetNumberAsDouble();
                            }
                        }
                    }
                }
            }
        }
    }
}  // namespace foray