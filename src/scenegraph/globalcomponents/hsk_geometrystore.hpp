#pragma once
#include "../../memory/hsk_managedbuffer.hpp"
#include "../../raytracing/hsk_blas.hpp"
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
        EType    Type  = {};

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
        inline Mesh(GeometryBufferSet* buffer) : mGeometryBufferSet(buffer) {}

        virtual ~Mesh(){};

        virtual void CmdDraw(VkCommandBuffer commandBuffer, GeometryBufferSet*& currentlyBoundSet);
        virtual void CmdDrawInstanced(VkCommandBuffer commandBuffer, GeometryBufferSet*& currentlyBoundSet, uint32_t instanceCount);

        virtual void BuildAccelerationStructure(const VkContext* context) { mBlas.Create(context, this); }

        HSK_PROPERTY_ALL(GeometryBufferSet)
        HSK_PROPERTY_ALL(Primitives)
        HSK_PROPERTY_GET(Blas)


      protected:
        GeometryBufferSet*     mGeometryBufferSet;
        std::vector<Primitive> mPrimitives;
        Blas                   mBlas;
    };

    class GeometryBufferSet
    {
      public:
        GeometryBufferSet();

        HSK_PROPERTY_ALL(Indices)
        HSK_PROPERTY_ALL(Vertices)
        HSK_PROPERTY_ALL(IndexCount)
        HSK_PROPERTY_ALL(VertexCount)

        void Init(const VkContext* context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices = std::vector<uint32_t>{});

        virtual bool CmdBindBuffers(VkCommandBuffer commandBuffer);

        inline virtual ~GeometryBufferSet()
        {
            mIndices.Cleanup();
            mVertices.Cleanup();
            mIndexCount  = 0;
            mVertexCount = 0;
        }

      protected:
        ManagedBuffer mIndices;
        ManagedBuffer mVertices;
        uint32_t      mIndexCount  = 0;
        uint32_t      mVertexCount = 0;
    };

    class GeometryStore : public GlobalComponent
    {
      public:
        GeometryStore();

        void Cleanup();

        virtual ~GeometryStore() { Cleanup(); }

        HSK_PROPERTY_ALL(BufferSets)
        HSK_PROPERTY_ALL(Meshes)

      protected:
        std::vector<std::unique_ptr<GeometryBufferSet>> mBufferSets;
        std::vector<std::unique_ptr<Mesh>>              mMeshes;
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