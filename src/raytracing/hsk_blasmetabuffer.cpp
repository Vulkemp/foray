#include "hsk_blasmetabuffer.hpp"
#include "../memory/hsk_descriptorsethelper.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"

namespace hsk {
    const std::unordered_map<const Blas*, uint32_t>& BlasMetaBuffer::CreateOrUpdate(const VkContext* context, const std::unordered_set<const Blas*>& entries)
    {
        mContext = context;
        mBufferOffsets.clear();

        uint64_t capacity = 0;

        for(auto blas : entries)
        {
            capacity += blas->GetMesh()->GetPrimitives().size();
        }

        std::vector<BlasGeometryMeta> bufferData(capacity);
        bufferData.resize(capacity);

        uint64_t offset = 0;

        // BLAS:       BLAS #0           | BLAS #1
        // Primitives: P0 | P1 | P2 | P3 | P0 | P1
        // Offsets     0                 | 4

        for(auto blas : entries)
        {
            mBufferOffsets[blas]   = offset;
            const auto& primitives = blas->GetMesh()->GetPrimitives();
            for(int32_t primitiveIndex = 0; primitiveIndex < primitives.size(); primitiveIndex++)
            {
                const Primitive& primitive          = primitives[primitiveIndex];
                bufferData[offset + primitiveIndex] = BlasGeometryMeta{.MaterialIndex = 0, .IndexBufferOffset = primitive.First};
            }
            offset += primitives.size();
        }

        VkDeviceSize newBufferSize = capacity * sizeof(BlasGeometryMeta);
        VkDeviceSize oldBufferSize = mBuffer.GetSize();

        if(newBufferSize > oldBufferSize)
        {
            mBuffer.Destroy();
            mBuffer.Create(mContext, VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, newBufferSize, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0,
                           "Blas Geometry Meta Buffer");
        }

        mBuffer.WriteDataDeviceLocal(bufferData.data(), newBufferSize);

        return mBufferOffsets;
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> BlasMetaBuffer::GetDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        mDescriptorInfos    = {mBuffer.GetVkDescriptorBufferInfo()};
        auto descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage, &mDescriptorInfos);
        return descriptorInfo;
    }
}  // namespace hsk