#include "foray_mesh.hpp"
#include "foray_scenedrawing.hpp"

namespace foray::scene {
    void Primitive::CmdDraw(VkCommandBuffer commandBuffer)
    {
        if(IsValid())
        {
            if(Type == EType::Index)
            {
                vkCmdDrawIndexed(commandBuffer, VertexOrIndexCount, 1, First, 0, 0);
            }
            else
            {
                vkCmdDraw(commandBuffer, VertexOrIndexCount, 1, First, 0);
            }
        }
    }

    void Primitive::CmdDrawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount)
    {
        if(IsValid())
        {
            if(Type == EType::Index)
            {
                vkCmdDrawIndexed(commandBuffer, VertexOrIndexCount, instanceCount, First, 0, 0);
            }
            else
            {
                vkCmdDraw(commandBuffer, VertexOrIndexCount, instanceCount, First, 0);
            }
        }
    }

    void Mesh::CmdDraw(SceneDrawInfo& drawInfo)
    {
        if(mPrimitives.size())
        {
            for(auto& primitive : mPrimitives)
            {
                primitive.CmdDraw(drawInfo.RenderInfo.GetCommandBuffer());
            }
        }
    }

    void Mesh::CmdDrawInstanced(SceneDrawInfo& drawInfo, uint32_t instanceCount)
    {
        if(mPrimitives.size())
        {
            for(auto& primitive : mPrimitives)
            {
                drawInfo.CmdPushConstant_MaterialIndex(primitive.MaterialIndex);
                primitive.CmdDrawInstanced(drawInfo.RenderInfo.GetCommandBuffer(), instanceCount);
            }
        }
    }
}  // namespace foray