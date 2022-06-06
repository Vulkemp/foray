#pragma once
#include "../../memory/hsk_managedbuffer.hpp"
#include "../hsk_component.hpp"
#include "../hsk_geo.hpp"

namespace hsk {


    class GeometryStore : public GlobalComponent, public Component::BeforeDrawCallback
    {
      public:
        GeometryStore();

        void Init(const std::vector<NVertex>& vertices, const std::vector<uint32_t>& indices = std::vector<uint32_t>{});

        inline bool CmdBindBuffers(VkCommandBuffer commandBuffer);

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;

        void Cleanup();

        virtual ~GeometryStore() { Cleanup(); }

      protected:
        ManagedBuffer mIndices;
        ManagedBuffer mVertices;
    };

    bool GeometryStore::CmdBindBuffers(VkCommandBuffer commandBuffer)
    {
        if(mVertices.GetAllocation())
        {
            const VkDeviceSize offsets[1]      = {0};
            VkBuffer           vertexBuffers[] = {mVertices.GetBuffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            if(mIndices.GetAllocation())
            {
                vkCmdBindIndexBuffer(commandBuffer, mIndices.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
            }
            return true;
        }
        return false;
    }
}  // namespace hsk