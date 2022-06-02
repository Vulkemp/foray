#include "hsk_material.hpp"
#include "../memory/hsk_managedbuffer.hpp"
#include "../memory/hsk_vmaHelpers.hpp"
#include "hsk_scene.hpp"

namespace hsk {

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
            HSK_THROWFMT("Unable to parse alpha mode {}", raw)
        }
    }


    void Material::InitFromTinyGltfMaterial(const tinygltf::Material& material)
    {
        // base info
        Name          = material.name;
        AlphaMode     = ParseTinyGltfAlphaMode(material.alphaMode);
        AlphaCutoff   = material.alphaCutoff;
        IsDoubleSided = material.doubleSided;

        // Pbr Base Info
        BaseColorFactor          = glm::vec4(material.pbrMetallicRoughness.baseColorFactor[0], material.pbrMetallicRoughness.baseColorFactor[1],
                                             material.pbrMetallicRoughness.baseColorFactor[2], material.pbrMetallicRoughness.baseColorFactor[3]);
        MetallicFactor           = material.pbrMetallicRoughness.metallicFactor;
        RoughnessFactor          = material.pbrMetallicRoughness.roughnessFactor;
        BaseColorTexture         = material.pbrMetallicRoughness.baseColorTexture.index;
        MetallicRoughnessTexture = material.pbrMetallicRoughness.metallicRoughnessTexture.index;

        // Aux Info
        EmissiveFactor  = glm::vec3(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]);
        EmissiveTexture = material.emissiveTexture.index;
        NormalTexture   = material.normalTexture.index;
    }

    MaterialBufferObject Material::MakeBufferObject()
    {
        MaterialBufferObject result{};
        result.BaseColorFactor               = BaseColorFactor;
        result.MetallicFactor                = MetallicFactor;
        result.EmissiveFactor                = EmissiveFactor;
        result.RoughnessFactor               = RoughnessFactor;
        result.BaseColorTextureIndex         = BaseColorTexture;
        result.MetallicRoughnessTextureIndex = MetallicRoughnessTexture;
        result.EmissiveTextureIndex          = EmissiveTexture;
        result.NormalTextureIndex            = NormalTexture;
        return result;
    }

    void MaterialBuffer::InitFromTinyGltfMaterials(const std::vector<tinygltf::Material>& materials)
    {
        mMaterialDescriptions.clear();
        mDeviceBuffer.GetVector().clear();
        for(int32_t i = 0; i < materials.size(); i++)
        {
            Material material = {};
            material.InitFromTinyGltfMaterial(materials[i]);
            mMaterialDescriptions.push_back(material);
            mDeviceBuffer.GetVector().push_back(material.MakeBufferObject());
        }

        UpdateBuffer();
    }
    void MaterialBuffer::Cleanup()
    {
        mDeviceBuffer.Cleanup();
        mMaterialDescriptions.clear();
        mDeviceBuffer.GetVector().clear();
    }
    void MaterialBuffer::UpdateBuffer()
    {
        mDeviceBuffer.SetContext(Context());
        mDeviceBuffer.GetDeviceBuffer().SetName("MaterialBuffer");

        mDeviceBuffer.InitOrUpdate();
    }
    void MaterialBuffer::WriteDescriptorSet(VkDescriptorSet set, uint32_t binding)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer                 = mDeviceBuffer.GetDeviceBuffer().GetBuffer();
        bufferInfo.offset                 = 0;
        bufferInfo.range                  = mDeviceBuffer.GetDeviceSize();

        VkWriteDescriptorSet writeOpInfo = {};
        writeOpInfo.sType                = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeOpInfo.dstSet               = set;
        writeOpInfo.dstBinding           = binding;
        writeOpInfo.descriptorCount      = 1;
        writeOpInfo.descriptorType       = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeOpInfo.pBufferInfo          = &bufferInfo;

        vkUpdateDescriptorSets(Context()->Device, 1, &writeOpInfo, 0, nullptr);
    }

}  // namespace hsk