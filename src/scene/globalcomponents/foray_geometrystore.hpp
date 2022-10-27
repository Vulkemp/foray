#pragma once
#include "../../core/foray_managedbuffer.hpp"
#include "../foray_component.hpp"
#include "../foray_geo.hpp"
#include "../foray_mesh.hpp"
#include <set>

namespace foray::scene {

    class GeometryStore : public GlobalComponent
    {
      public:
        GeometryStore();

        void InitOrUpdate();

        void Destroy();

        FORAY_PROPERTY_ALL(Indices)
        FORAY_PROPERTY_ALL(Vertices)
        FORAY_PROPERTY_ALL(IndicesBuffer)
        FORAY_PROPERTY_ALL(VerticesBuffer)

        virtual ~GeometryStore() { Destroy(); }

        FORAY_PROPERTY_ALL(Meshes)

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