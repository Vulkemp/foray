#pragma once
#include "../../memory/hsk_descriptorsethelper.hpp"
#include "../../memory/hsk_managedbuffer.hpp"
#include "../../raytracing/hsk_blas.hpp"
#include "../hsk_component.hpp"
#include "../hsk_geo.hpp"
#include "../hsk_mesh.hpp"
#include <set>

namespace hsk {

    class GeometryStore : public GlobalComponent
    {
      public:
        GeometryStore();

        void InitOrUpdate();

        void Destroy();

        HSK_PROPERTY_ALL(Indices)
        HSK_PROPERTY_ALL(Vertices)
        HSK_PROPERTY_ALL(IndicesBuffer)
        HSK_PROPERTY_ALL(VerticesBuffer)

        virtual ~GeometryStore() { Destroy(); }

        HSK_PROPERTY_ALL(Meshes)

        bool                                                 CmdBindBuffers(VkCommandBuffer commandBuffer);
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
}  // namespace hsk