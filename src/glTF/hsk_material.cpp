#include "hsk_material.hpp"
#include "../memory/hsk_vmaHelpers.hpp"
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
        EmissiveFactor   = glm::vec3(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]);
        EmissiveTexture  = material.emissiveTexture.index;
        NormalTexture    = material.normalTexture.index;
        OcclusionTexture = material.occlusionTexture.index;
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
        result.OcclusionTextureIndex         = OcclusionTexture;
        result.NormalTextureIndex            = NormalTexture;
        return result;
    }

    void MaterialBuffer::InitFromTinyGltfMaterials(const std::vector<tinygltf::Material>& materials)
    {
        mMaterialDescriptions.clear();
        mBufferArray.clear();
        for(int32_t i = 0; i < materials.size(); i++)
        {
            Material description = {};
            description.InitFromTinyGltfMaterial(materials[i]);
            mMaterialDescriptions.push_back(description);
            mBufferArray.push_back(description.MakeBufferObject());
        }

        UpdateBuffer();
    }
    void MaterialBuffer::Cleanup() { DestroyBuffer(); }
    void MaterialBuffer::CreateBuffer(VkDeviceSize size)
    {
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        mBuffer.Init(Context()->Allocator, VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, allocInfo, size);
        mBufferSize = size;
    }
    void MaterialBuffer::UpdateBuffer()
    {
        size_t bufferObjectSize = sizeof(MaterialBufferObject);
        size_t bufferSize       = bufferObjectSize * mBufferArray.size();

        if(static_cast<VkDeviceSize>(bufferSize) > mBufferSize)
        {
            DestroyBuffer();
            CreateBuffer(bufferSize + (bufferSize / 4));
        }

        // Get a staging buffer setup, init with buffer array data

        ManagedBuffer stagingBuffer(Context()->Allocator);
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.flags                   = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        stagingBuffer.Init(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT, allocInfo, bufferSize, mBufferArray.data());

        // copy to GPU buffer

        VkCommandBuffer copyCmdBuf = createCommandBuffer(Context()->Device, Context()->TransferCommandPool, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copyRegion = {};
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(copyCmdBuf, stagingBuffer.GetBuffer(), mBuffer.GetBuffer(), 1, &copyRegion);

        flushCommandBuffer(Context()->Device, Context()->TransferCommandPool, copyCmdBuf, Context()->TransferQueue);

        stagingBuffer.Destroy();
    }
    void MaterialBuffer::DestroyBuffer() { mBuffer.Destroy(); }

}  // namespace hsk