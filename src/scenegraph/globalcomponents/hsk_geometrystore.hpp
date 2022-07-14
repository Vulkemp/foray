#pragma once
#include "../../memory/hsk_managedbuffer.hpp"
#include "../../raytracing/hsk_blas.hpp"
#include "../hsk_component.hpp"
#include "../hsk_geo.hpp"
#include <set>

namespace hsk {

    /// @brief Reference into a vertex or index buffer which combine into a valid draw call
    struct Primitive
    {
        enum class EType
        {
            Vertex,
            Index
        };
        EType    Type  = {};
        uint32_t First = 0;
        uint32_t Count = 0;

        inline Primitive() {}
        inline Primitive(EType type, uint32_t first, uint32_t count);

        bool        IsValid() const { return Count > 0; }
        inline void CmdDraw(VkCommandBuffer commandBuffer);
    };

    class Mesh
    {
      public:
        inline Mesh() {}
        inline Mesh(GeometryBufferSet* buffer) : mGeometryBufferSet(buffer) {}

        virtual ~Mesh(){};

        virtual void CmdDraw(VkCommandBuffer commandBuffer, GeometryBufferSet*& currentlyBoundSet);

        virtual void BuildAccelerationStructure(const VkContext* context) { mBlas.Create(context, this); }

        HSK_PROPERTY_ALL(GeometryBufferSet)
        HSK_PROPERTY_ALL(Primitives)
        HSK_PROPERTY_ALL(HighestIndexValue)
        HSK_PROPERTY_GET(Blas)


      protected:
        GeometryBufferSet*     mGeometryBufferSet;
        std::vector<Primitive> mPrimitives;
        Blas                   mBlas;
        uint32_t               mHighestIndexValue{}; // TODO: required for AS building, but shouldn't this instead be acquired from an index buffer directly?
    };

    class GeometryBufferSet
    {
      public:
        GeometryBufferSet();

        HSK_PROPERTY_ALL(Indices)
        HSK_PROPERTY_ALL(Vertices)

        void Init(const VkContext* context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices = std::vector<uint32_t>{});

        virtual bool CmdBindBuffers(VkCommandBuffer commandBuffer);

        inline virtual ~GeometryBufferSet()
        {
            mIndices.Cleanup();
            mVertices.Cleanup();
        }

      protected:
        ManagedBuffer mIndices;
        ManagedBuffer mVertices;
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

    inline Primitive::Primitive(EType type, uint32_t first, uint32_t count) : Type(type), First(first), Count(count) {}

    inline void Primitive::CmdDraw(VkCommandBuffer commandBuffer)
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