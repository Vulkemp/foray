#pragma once
#include "../as/foray_blas.hpp"
#include "../foray_basics.hpp"
#include "../scene/foray_geo.hpp"
#include "foray_scene_declares.hpp"

namespace foray::scene {

    /// @brief "An object binding indexed or non-indexed geometry with a material." according to the glTF spec.
    /// It's a subset of a mesh and has its own set of vertices/indices as well as its own material.
    /// All mesh data is contained in one big buffer, so "First" is used to get the correct offset into the buffer.
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

        std::vector<foray::scene::Vertex> Vertices;
        std::vector<uint32_t>             Indices;

        inline Primitive() {}
        inline Primitive(
            EType type, uint32_t first, uint32_t count, int32_t materialIndex, int32_t highestRef, std::vector<foray::scene::Vertex>& vertices, std::vector<uint32_t> indices)
            : Type(type), First(first), VertexOrIndexCount(count), MaterialIndex(materialIndex), HighestReferencedIndex(highestRef), Vertices(vertices), Indices(indices)
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

        virtual void BuildAccelerationStructure(core::Context* context, gcomp::GeometryStore* store) { mBlas.CreateOrUpdate(context, this, store); }

        FORAY_PROPERTY_R(Primitives)
        FORAY_GETTER_MR(Blas)
        FORAY_PROPERTY_R(Name)

      protected:
        std::vector<Primitive> mPrimitives;
        as::Blas               mBlas;
        std::string            mName = "";
    };
}  // namespace foray::scene