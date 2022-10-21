#include "foray_geometrymetabuffer.hpp"
#include "../core/foray_descriptorsethelper.hpp"
#include "../scene/foray_mesh.hpp"
#include "foray_blas.hpp"

namespace foray::as {
    const std::unordered_map<const Blas*, uint32_t>& GeometryMetaBuffer::CreateOrUpdate(core::Context* context, const std::unordered_set<const Blas*>& entries)
    {
        // STEP #0   Reset State
        mContext = context;
        mBufferOffsets.clear();

        // STEP #1   Calculate required capacity

        uint64_t capacity = 0;

        for(auto blas : entries)
        {
            capacity += blas->GetMesh()->GetPrimitives().size();
        }

        // STEP #2   Write array data

        std::vector<GeometryMeta> bufferData(capacity);

        uint64_t offset = 0;

        for(auto blas : entries)
        {
            mBufferOffsets[blas]   = offset;
            const auto& primitives = blas->GetMesh()->GetPrimitives();
            for(int32_t primitiveIndex = 0; primitiveIndex < primitives.size(); primitiveIndex++)
            {
                const scene::Primitive& primitive   = primitives[primitiveIndex];
                bufferData[offset + primitiveIndex] = GeometryMeta{.MaterialIndex = primitive.MaterialIndex, .IndexBufferOffset = primitive.First};
            }
            offset += primitives.size();
        }

        // STEP #3    Recreate Buffer (if needed)

        VkDeviceSize newBufferSize = capacity * sizeof(GeometryMeta);
        VkDeviceSize oldBufferSize = mBuffer.GetSize();

        if(newBufferSize > oldBufferSize)
        {
            mBuffer.Destroy();
            mBuffer.Create(mContext, VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, newBufferSize,
                           VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Blas Geometry Meta Buffer");
        }

        // STEP #4    Copy array to device

        mBuffer.WriteDataDeviceLocal(bufferData.data(), newBufferSize);

        return mBufferOffsets;
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GeometryMetaBuffer::GetDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        mDescriptorInfos    = {mBuffer.GetVkDescriptorBufferInfo()};
        auto descriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage, &mDescriptorInfos);
        return descriptorInfo;
    }
}  // namespace foray::as