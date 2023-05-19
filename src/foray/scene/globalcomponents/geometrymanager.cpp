#include "geometrymanager.hpp"
#include "../scene.hpp"

namespace foray::scene::gcomp {

    bool GeometryStore::CmdBindBuffers(VkCommandBuffer commandBuffer)
    {
        if(mVerticesBuffer)
        {
            const VkDeviceSize offsets[1]      = {0};
            VkBuffer           vertexBuffers[] = {mVerticesBuffer->GetBuffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            if(mIndicesBuffer)
            {
                vkCmdBindIndexBuffer(commandBuffer, mIndicesBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
            }
            return true;
        }
        return false;
    }

    GeometryStore::GeometryStore()
    {
    }

    void GeometryStore::InitOrUpdate()
    {
        VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        // enable calls to GetBufferDeviceAdress & using the buffer as source for acceleration structure building
        bufferUsageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        VkDeviceSize verticesSize = mVertices.size() * sizeof(Vertex);
        VkDeviceSize indicesSize  = mIndices.size() * sizeof(uint32_t);
        VkDeviceSize verticesBufferSize = mVerticesBuffer ? mVerticesBuffer->GetSize() : 0;
        VkDeviceSize indicesBufferSize = mIndicesBuffer ? mIndicesBuffer->GetSize() : 0;

        if(verticesSize > verticesBufferSize)
        {
            mVerticesBuffer.New(GetContext(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | bufferUsageFlags, verticesSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Vertices");
        }
        if(indicesSize > indicesBufferSize)
        {
            mIndicesBuffer.New(GetContext(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | bufferUsageFlags, indicesSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, "Indices");
        }
        mVerticesBuffer->WriteDataDeviceLocal(mVertices.data(), verticesSize);
        mIndicesBuffer->WriteDataDeviceLocal(mIndices.data(), indicesSize);
    }
}  // namespace foray::scene