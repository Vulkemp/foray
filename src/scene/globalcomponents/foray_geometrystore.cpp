#include "foray_geometrystore.hpp"
#include "../foray_scene.hpp"

namespace foray::scene {

    bool GeometryStore::CmdBindBuffers(VkCommandBuffer commandBuffer)
    {
        if(mVerticesBuffer.GetAllocation())
        {
            const VkDeviceSize offsets[1]      = {0};
            VkBuffer           vertexBuffers[] = {mVerticesBuffer.GetBuffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            if(mIndicesBuffer.GetAllocation())
            {
                vkCmdBindIndexBuffer(commandBuffer, mIndicesBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
            }
            return true;
        }
        return false;
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GeometryStore::GetVertexBufferDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        auto descriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        mDescriptorBufferInfosVertices.resize(1);
        mVerticesBuffer.FillVkDescriptorBufferInfo(&mDescriptorBufferInfosVertices[0]);
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage, &mDescriptorBufferInfosVertices);
        return descriptorInfo;
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> GeometryStore::GetIndexBufferDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        auto descriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        mDescriptorBufferInfosIndices.resize(1);
        mIndicesBuffer.FillVkDescriptorBufferInfo(&mDescriptorBufferInfosIndices[0]);
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage, &mDescriptorBufferInfosIndices);
        return descriptorInfo;
    }

    GeometryStore::GeometryStore()
    {
        mIndicesBuffer.SetName("Indices");
        mVerticesBuffer.SetName("Vertices");
    }

    void GeometryStore::InitOrUpdate()
    {
        VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

#ifndef DISABLE_RT_EXTENSIONS
        // enable calls to GetBufferDeviceAdress & using the buffer as source for acceleration structure building
        bufferUsageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
#endif

        VkDeviceSize verticesSize = mVertices.size() * sizeof(Vertex);
        VkDeviceSize indicesSize  = mIndices.size() * sizeof(uint32_t);

        if(verticesSize > mVerticesBuffer.GetSize())
        {
            mVerticesBuffer.Destroy();
            mVerticesBuffer.Create(GetContext(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | bufferUsageFlags, verticesSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        }
        if(indicesSize > mIndicesBuffer.GetSize())
        {
            mIndicesBuffer.Destroy();
            mIndicesBuffer.Create(GetContext(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | bufferUsageFlags, indicesSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        }
        mVerticesBuffer.WriteDataDeviceLocal(mVertices.data(), verticesSize);
        mIndicesBuffer.WriteDataDeviceLocal(mIndices.data(), indicesSize);
    }

    void GeometryStore::Destroy()
    {
        mMeshes.clear();
        mIndices.clear();
        mVertices.clear();
        mVerticesBuffer.Destroy();
        mIndicesBuffer.Destroy();
    }
}  // namespace foray::scene