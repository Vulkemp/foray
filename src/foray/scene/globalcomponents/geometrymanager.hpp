#pragma once
#include "../../core/managedbuffer.hpp"
#include "../component.hpp"
#include "../geo.hpp"
#include "../mesh.hpp"
#include <set>

namespace foray::scene::gcomp {

    /// @brief Stores all geometry in a single set of index and vertex buffers
    class GeometryStore : public GlobalComponent
    {
      public:
        GeometryStore();

        /// @brief Rewrites Indices and Vertices from CPU side storage to the GPU buffers
        void InitOrUpdate();

        FORAY_PROPERTY_R(Indices)
        FORAY_PROPERTY_R(Vertices)
        FORAY_GETTER_MEM(IndicesBuffer)
        FORAY_GETTER_MEM(VerticesBuffer)

        virtual ~GeometryStore() = default;

        FORAY_PROPERTY_R(Meshes)

        bool                   CmdBindBuffers(VkCommandBuffer commandBuffer);
        vk::DescriptorBufferInfo GetVertexBufferDescriptorInfo() const { return mVerticesBuffer->GetVkDescriptorBufferInfo(); }
        vk::DescriptorBufferInfo GetIndexBufferDescriptorInfo() const { return mIndicesBuffer->GetVkDescriptorBufferInfo(); }

      protected:
        Local<core::ManagedBuffer> mIndicesBuffer;
        Local<core::ManagedBuffer> mVerticesBuffer;
        std::vector<Vertex>        mVertices;
        std::vector<uint32_t>      mIndices;

        std::vector<Heap<Mesh>> mMeshes;
    };
}  // namespace foray::scene::gcomp