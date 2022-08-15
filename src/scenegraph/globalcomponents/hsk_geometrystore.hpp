#pragma once
#include "../../memory/hsk_managedbuffer.hpp"
#include "../../raytracing/hsk_blas.hpp"
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../hsk_component.hpp"
#include "../hsk_geo.hpp"
#include <set>

namespace hsk {

    /// @brief Used to index into a vertex or index buffer which combine into a valid draw call.
    struct Primitive
    {
        enum class EType
        {
            Vertex,
            Index
        };
        EType Type = {};

        /// @brief Index to the first index/vertex in a buffer.
        uint32_t First = 0;
        /// @brief Number of indices/vertices used for this primitive.
        uint32_t VertexOrIndexCount = 0;

        inline Primitive() {}
        inline Primitive(EType type, uint32_t first, uint32_t count);

        bool        IsValid() const { return VertexOrIndexCount > 0; }
        inline void CmdDraw(VkCommandBuffer commandBuffer);
        inline void CmdDrawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount);
    };

    class Mesh
    {
      public:
        inline Mesh() {}

        virtual ~Mesh(){};

        virtual void CmdDraw(VkCommandBuffer commandBuffer);
        virtual void CmdDrawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount);

        virtual void BuildAccelerationStructure(const VkContext* context, GeometryStore* store) { mBlas.Create(context, this, store); }

        HSK_PROPERTY_ALL(Primitives)
        HSK_PROPERTY_GET(Blas)


      protected:
        std::vector<Primitive> mPrimitives;
        Blas                   mBlas;
    };

    class GeometryStore : public GlobalComponent
    {
      public:
        GeometryStore();

        void InitOrUpdate();

        void Cleanup();

        HSK_PROPERTY_ALL(Indices)
        HSK_PROPERTY_ALL(Vertices)
        HSK_PROPERTY_ALL(IndicesBuffer)
        HSK_PROPERTY_ALL(VerticesBuffer)

        virtual ~GeometryStore() { Cleanup(); }

        HSK_PROPERTY_ALL(Meshes)

        bool CmdBindBuffers(VkCommandBuffer commandBuffer);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetVertexBufferDescriptorInfo(VkShaderStageFlags shaderStage);
        std::shared_ptr<DescriptorSetHelper::DescriptorInfo> GetIndexBufferDescriptorInfo(VkShaderStageFlags shaderStage);

      protected:
        ManagedBuffer         mIndicesBuffer;
        ManagedBuffer         mVerticesBuffer;
        std::vector<Vertex>   mVertices;
        std::vector<uint32_t> mIndices;

         std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mDescriptorInfoVertexBuffer;
         std::shared_ptr<DescriptorSetHelper::DescriptorInfo> mDescriptorInfoIndexBuffer;
         std::vector<VkDescriptorBufferInfo>                  mDescriptorBufferInfosVertices;
         std::vector<VkDescriptorBufferInfo>                  mDescriptorBufferInfosIndices;

        std::vector<std::unique_ptr<Mesh>> mMeshes;
    };

    inline Primitive::Primitive(EType type, uint32_t first, uint32_t count) : Type(type), First(first), VertexOrIndexCount(count) {}

    inline void Primitive::CmdDraw(VkCommandBuffer commandBuffer)
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

    inline void Primitive::CmdDrawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount)
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
}  // namespace hsk