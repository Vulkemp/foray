#pragma once
#include "../../core/foray_managedbuffer.hpp"
#include "../foray_component.hpp"
#include "../foray_geo.hpp"
#include "../foray_mesh.hpp"
#include <set>

namespace foray::scene::gcomp {

    /// @brief Stores all geometry in a single set of index and vertex buffers
    class GeometryStore : public GlobalComponent
    {
      public:
        GeometryStore();

        /// @brief Rewrites Indices and Vertices from CPU side storage to the GPU buffers
        void InitOrUpdate();

        void Destroy();

        FORAY_PROPERTY_R(Indices)
        FORAY_PROPERTY_R(Vertices)
        FORAY_PROPERTY_R(IndicesBuffer)
        FORAY_PROPERTY_R(VerticesBuffer)

        virtual ~GeometryStore() { Destroy(); }

        FORAY_PROPERTY_R(Meshes)

        bool                   CmdBindBuffers(VkCommandBuffer commandBuffer);
        VkDescriptorBufferInfo GetVertexBufferDescriptorInfo() const { return mVerticesBuffer.GetVkDescriptorBufferInfo(); }
        VkDescriptorBufferInfo GetIndexBufferDescriptorInfo() const { return mIndicesBuffer.GetVkDescriptorBufferInfo(); }

      protected:
        core::ManagedBuffer   mIndicesBuffer;
        core::ManagedBuffer   mVerticesBuffer;
        std::vector<Vertex>   mVertices;
        std::vector<uint32_t> mIndices;

        std::vector<std::unique_ptr<Mesh>> mMeshes;
    };
}  // namespace foray::scene