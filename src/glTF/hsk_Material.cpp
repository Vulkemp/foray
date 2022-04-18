#include "hsk_Material.hpp"
#include "hsk_scene.hpp"

namespace hsk {

    Material::Material() {}
    Material::Material(Scene* scene) : SceneComponent(scene) {}

    Material::EAlphaMode Material::ParseTinyGltfAlphaMode(std::string_view raw)
    {
        // see https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_material_alphamode
        if(raw == "OPAQUE")
        {
            return Material::EAlphaMode::Opaque;
        }
        else if(raw == "MASK")
        {
            return Material::EAlphaMode::Mask;
        }
        else if(raw == "BLEND")
        {
            return Material::EAlphaMode::Blend;
        }
        else
        {
            throw Exception("Unable to parse alpha mode {}", raw);
        }
    }

    template <typename TTexInfo>
    void resolveTinygltfMaterial(const TTexInfo& texInfo, Scene* scene, Texture*& out_texture, uint8_t& out_texcoord)
    {
        if(texInfo.index >= 0)
        {
            out_texture  = scene->GetTextureByIndex(texInfo.index);
            out_texcoord = texInfo.texCoord;
        }
        else
        {
            out_texture  = nullptr;
            out_texcoord = 0;
        }
    }

    void Material::InitFromTinyGltfMaterial(tinygltf::Material& material)
    {
        // base info
        Name = material.name;
        AlphaMode        = ParseTinyGltfAlphaMode(material.alphaMode);
        AlphaCutoff      = material.alphaCutoff;
        IsDoubleSided    = material.doubleSided;

        // Pbr Base Info
        BaseColorFactor = glm::vec4(material.pbrMetallicRoughness.baseColorFactor[0], material.pbrMetallicRoughness.baseColorFactor[1],
                                     material.pbrMetallicRoughness.baseColorFactor[2], material.pbrMetallicRoughness.baseColorFactor[3]);
        MetallicFactor  = material.pbrMetallicRoughness.metallicFactor;
        RoughnessFactor = material.pbrMetallicRoughness.roughnessFactor;
        resolveTinygltfMaterial(material.pbrMetallicRoughness.baseColorTexture, mOwningScene, BaseColorTexture, TexCoordIndices.BaseColor);
        resolveTinygltfMaterial(material.pbrMetallicRoughness.metallicRoughnessTexture, mOwningScene, MetallicRoughnessTexture, TexCoordIndices.MetallicRoughness);

        // Aux Info
        EmissiveFactor   = glm::vec3(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]);
        resolveTinygltfMaterial(material.emissiveTexture, mOwningScene, EmissiveTexture, TexCoordIndices.Emissive);
        resolveTinygltfMaterial(material.normalTexture, mOwningScene, NormalTexture, TexCoordIndices.Normal);
        resolveTinygltfMaterial(material.occlusionTexture, mOwningScene, OcclusionTexture, TexCoordIndices.Occlusion);
        Loaded = true;
    }

}  // namespace hsk