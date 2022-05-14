#pragma once
#include "../memory/hsk_managedubo.hpp"
#include "glm/glm.hpp"
#include "hsk_boundingBox.hpp"
#include "hsk_glTF_declares.hpp"
#include "hsk_scenecomponent.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "hsk_geo.hpp"

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

namespace hsk {
    struct Primitive
    {
        uint32_t    FirstIndex  = 0;
        uint32_t    IndexCount  = 0;
        uint32_t    VertexCount = 0;
        Material*   Mat;
        bool        HasIndices = false;
        BoundingBox Bounds     = {};
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material* material);
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

    class Mesh : public SceneComponent, public NoMoveDefaults
    {
      public:
        struct UniformBlock
        {
            glm::mat4 matrix                      = {};
            glm::mat4 jointMatrix[MAX_NUM_JOINTS] = {};
            float     jointcount                  = 0;
        };

        Mesh();
        Mesh(Scene* scene);
        void InitFromTinyGltfMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, uint32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
        ~Mesh();
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
        void Cleanup();
        void Update(const glm::mat4& mat, Skin* skin);

        HSK_PROPERTY_CGET(Primitives)
        HSK_PROPERTY_ALL(Bounds)
        HSK_PROPERTY_ALL(AxisAlignedBoundingBox)

        ManagedUbo<UniformBlock>*       Ubo() { return mUbo.get(); }
        const ManagedUbo<UniformBlock>* Ubo() const { return mUbo.get(); }

        HSK_PROPERTY_CGET(DescriptorSet)

      protected:
        std::vector<std::unique_ptr<Primitive>> mPrimitives             = {};
        BoundingBox                             mBounds            = {};
        BoundingBox                             mAxisAlignedBoundingBox = {};
        VertexInputStateBuilder                 mVertexInputStateBuilder{};

        std::unique_ptr<ManagedUbo<UniformBlock>> mUbo;
        VkDescriptorSet                           mDescriptorSet;
    };

}  // namespace hsk