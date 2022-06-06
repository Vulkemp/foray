#include "hsk_geometrystore.hpp"
#include "../hsk_scene.hpp"

namespace hsk {
    GeometryStore::GeometryStore()
    {
        mIndices.SetName("Indices");
        mVertices.SetName("Vertices");
    }

    void GeometryStore::Init(const std::vector<NVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        if(vertices.size())
        {
            VkDeviceSize bufferSize = vertices.size() * sizeof(NVertex);
            mVertices.Create(GetContext(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
            mVertices.WriteDataDeviceLocal(vertices.data(), bufferSize);
        }
        if(indices.size())
        {
            VkDeviceSize bufferSize = indices.size() * sizeof(uint32_t);
            mIndices.Create(GetContext(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, bufferSize, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
            mIndices.WriteDataDeviceLocal(indices.data(), bufferSize);
        }
    }

    void GeometryStore::BeforeDraw(const FrameRenderInfo& renderInfo) { CmdBindBuffers(renderInfo.GetCommandBuffer()); }

    void GeometryStore::Cleanup(){
        mIndices.Destroy();
        mVertices.Destroy();
    }
}  // namespace hsk