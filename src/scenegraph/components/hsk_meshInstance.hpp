#pragma once
#include "../hsk_component.hpp"
#include <glm/glm.hpp>

namespace hsk {
    struct NPrimitive
    {
        uint32_t FirstIndex  = 0;
        uint32_t IndexCount  = 0;
        uint32_t VertexCount = 0;

        inline NPrimitive() {}
        inline NPrimitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount);

        bool        IsValid() const { return (IndexCount + VertexCount) > 0; }
        bool        HasIndices() const { return IndexCount > 0; }
        inline void Draw(VkCommandBuffer commandBuffer);
    };

    class NMeshInstance : public Component, public Component::BeforeDrawCallback, public Component::DrawCallback
    {
      public:
        inline virtual ~NMeshInstance() {}

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;
        virtual void Draw(SceneDrawInfo& drawInfo) override;

      protected:
        int32_t                 mIndex      = 0;
        std::vector<NPrimitive> mPrimitives = {};
    };

    inline NPrimitive::NPrimitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount) : FirstIndex(firstIndex), IndexCount(indexCount), VertexCount(vertexCount) {}

    inline void NPrimitive::Draw(VkCommandBuffer commandBuffer)
    {
        if(IsValid())
        {
            if(HasIndices())
            {
                vkCmdDrawIndexed(commandBuffer, IndexCount, 1, FirstIndex, 0, 0);
            }
            else
            {
                vkCmdDraw(commandBuffer, VertexCount, 1, 0, 0);
            }
        }
    }
}  // namespace hsk