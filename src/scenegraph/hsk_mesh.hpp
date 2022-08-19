#pragma once
#include "../hsk_basics.hpp"
#include "../raytracing/hsk_blas.hpp"
#include "hsk_scenegraph_declares.hpp"

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
        /// @brief Index into the material buffer. Negative will cause use of fallback material
        int32_t MaterialIndex = 0;
        /// @brief The highest index into the vertex buffer referenced by this primitive. Used in Blas creation
        uint32_t HighestReferencedIndex = 0;

        inline Primitive() {}
        inline Primitive(EType type, uint32_t first, uint32_t count, int32_t materialIndex, int32_t highestRef)
            : Type(type), First(first), VertexOrIndexCount(count), MaterialIndex(materialIndex), HighestReferencedIndex(highestRef)
        {
        }

        bool IsValid() const { return VertexOrIndexCount > 0; }
        void CmdDraw(VkCommandBuffer commandBuffer);
        void CmdDrawInstanced(VkCommandBuffer commandBuffer, uint32_t instanceCount);
    };

    class Mesh
    {
      public:
        inline Mesh() {}

        virtual ~Mesh(){};

        virtual void CmdDraw(SceneDrawInfo& drawInfo);
        virtual void CmdDrawInstanced(SceneDrawInfo& drawInfo, uint32_t instanceCount);

        virtual void BuildAccelerationStructure(const VkContext* context, GeometryStore* store) { mBlas.CreateOrUpdate(context, this, store); }

        HSK_PROPERTY_ALL(Primitives)
        HSK_PROPERTY_GET(Blas)


      protected:
        std::vector<Primitive> mPrimitives;
        Blas                   mBlas;
    };
}  // namespace hsk