#pragma once
#include "glm/glm.hpp"
#include "hsk_boundingBox.hpp"
#include "hsk_glTF_declares.hpp"
#include <tinygltf/tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

namespace hsk {
    class Primitive
    {
      public:
        uint32_t    firstIndex = 0;
        uint32_t    indexCount = 0;
        uint32_t    vertexCount = 0;
        Material&   material;
        bool        hasIndices = false;
        BoundingBox bb = {};
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material);
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

    class Mesh : public SceneComponent, public NoMoveDefaults
    {
      public:
        std::vector<Primitive*> mPrimitives             = {};
        BoundingBox             mBoundingBox            = {};
        BoundingBox             mAxisAlignedBoundingBox = {};
        struct UniformBuffer
        {
            VkBuffer               buffer        = nullptr;
            VmaAllocation          allocation    = nullptr;
            VkDescriptorBufferInfo descriptor    = {};
            VkDescriptorSet        descriptorSet = nullptr;
            void*                  mapped        = nullptr;
        } uniformBuffer = {};
        struct UniformBlock
        {
            glm::mat4 matrix                      = {};
            glm::mat4 jointMatrix[MAX_NUM_JOINTS] = {};
            float     jointcount                  = 0;
        } uniformBlock = {};

        Mesh();
        Mesh(Scene* scene);
        void InitFromTinyGltfMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
        ~Mesh();
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

}  // namespace hsk