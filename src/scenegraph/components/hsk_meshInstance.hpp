#pragma once
#include "../hsk_component.hpp"
#include <glm/glm.hpp>

namespace hsk {
    /// @brief Reference into a vertex or index buffer which combine into a valid draw call
    struct NPrimitive
    {
        enum class EType
        {
            Vertex,
            Index
        };
        EType    Type  = {};
        uint32_t First = 0;
        uint32_t Count = 0;

        inline NPrimitive() {}
        inline NPrimitive(EType type, uint32_t first, uint32_t count);

        bool        IsValid() const { return Count > 0; }
        inline void Draw(VkCommandBuffer commandBuffer);
    };

    class NMeshInstance : public NodeComponent, public Component::BeforeDrawCallback, public Component::DrawCallback
    {
      public:
        inline virtual ~NMeshInstance() {}

        virtual void BeforeDraw(const FrameRenderInfo& renderInfo) override;
        virtual void Draw(SceneDrawInfo& drawInfo) override;

      protected:
        int32_t                 mIndex       = 0;
        int32_t                 mBufferIndex = 0;
        std::vector<NPrimitive> mPrimitives  = {};
    };

    inline NPrimitive::NPrimitive(EType type, uint32_t first, uint32_t count) : Type(type), First(first), Count(count) {}

    inline void NPrimitive::Draw(VkCommandBuffer commandBuffer)
    {
        if(IsValid())
        {
            if(Type == EType::Index)
            {
                vkCmdDrawIndexed(commandBuffer, Count, 1, First, 0, 0);
            }
            else
            {
                vkCmdDraw(commandBuffer, Count, 1, First, 0);
            }
        }
    }
}  // namespace hsk