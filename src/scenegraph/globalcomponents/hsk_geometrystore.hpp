#pragma once
#include "../../memory/hsk_managedbuffer.hpp"
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
        inline Mesh(GeometryBufferSet* buffer) : mBuffer(buffer) {}

        virtual void CmdDraw(VkCommandBuffer commandBuffer, GeometryBufferSet*& currentlyBoundSet);

        HSK_PROPERTY_ALL(Buffer)
        HSK_PROPERTY_ALL(Primitives)

      protected:
        GeometryBufferSet*      mBuffer;
        std::vector<Primitive> mPrimitives;
    };

    class GeometryBufferSet
    {
      public:
        GeometryBufferSet();

        HSK_PROPERTY_ALL(Indices)
        HSK_PROPERTY_ALL(Vertices)

        void Init(const VkContext* context, const std::vector<NVertex>& vertices, const std::vector<uint32_t>& indices = std::vector<uint32_t>{});

        virtual bool CmdBindBuffers(VkCommandBuffer commandBuffer);

        inline virtual ~GeometryBufferSet()
        {
            mIndices.Destroy();
            mVertices.Destroy();
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