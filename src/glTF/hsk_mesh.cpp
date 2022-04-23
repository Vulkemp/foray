#include "hsk_mesh.hpp"
#include "../hsk_vkHelpers.hpp"
#include "../hsk_vmaHelpers.hpp"
#include "hsk_scene.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace hsk {
    Mesh::Mesh() {}

    Mesh::Mesh(Scene* scene) : SceneComponent(scene) {}

    Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material)
        : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material)
    {
        hasIndices = indexCount > 0;
    };

    void Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        bb.min   = min;
        bb.max   = max;
        bb.valid = true;
    }

    void Mesh::InitFromTinyGltfMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
    {
        // TODO: This function is incredibly ugly

        // this->uniformBlock.matrix = matrix; TODO set node matrix


        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;

        uniformBuffer.Buffer.Init(Context()->Allocator, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, allocInfo, sizeof(uniformBlock), &uniformBlock);

        // AssertVkResult(vmaMapMemory(Context()->Allocator, uniformBuffer.allocation, &uniformBuffer.mapped));

        uniformBuffer.descriptor = {uniformBuffer.Buffer.Buffer(), 0, sizeof(uniformBlock)};
        for(size_t j = 0; j < mesh.primitives.size(); j++)
        {
            const tinygltf::Primitive& primitive   = mesh.primitives[j];
            uint32_t                   indexStart  = static_cast<uint32_t>(indexBuffer.size());
            uint32_t                   vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t                   indexCount  = 0;
            uint32_t                   vertexCount = 0;
            glm::vec3                  posMin{};
            glm::vec3                  posMax{};
            bool                       hasSkin    = false;
            bool                       hasIndices = primitive.indices > -1;
            // Vertices
            {
                const float* bufferPos          = nullptr;
                const float* bufferNormals      = nullptr;
                const float* bufferTexCoordSet0 = nullptr;
                const float* bufferTexCoordSet1 = nullptr;
                const void*  bufferJoints       = nullptr;
                const float* bufferWeights      = nullptr;

                int posByteStride;
                int normByteStride;
                int uv0ByteStride;
                int uv1ByteStride;
                int jointByteStride;
                int weightByteStride;

                int jointComponentType;

                // Position attribute is required
                assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                const tinygltf::Accessor&   posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& posView     = model.bufferViews[posAccessor.bufferView];
                bufferPos                               = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                posMin                                  = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                posMax                                  = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                vertexCount                             = static_cast<uint32_t>(posAccessor.count);

                auto tinygltf_GetTypeSizeInBytes = [](const tinygltf::Accessor& posAccessor) {
                    return tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(posAccessor.componentType)) * tinygltf::GetNumComponentsInType(posAccessor.type);
                };

                posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(posAccessor);

                if(primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& normView     = model.bufferViews[normAccessor.bufferView];
                    bufferNormals  = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                    normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(normAccessor);
                }

                if(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& uvView     = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet0                     = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(uvAccessor);
                }
                if(primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                    const tinygltf::BufferView& uvView     = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet1                     = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(uvAccessor);
                }

                // Skinning
                // Joints
                if(primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
                    const tinygltf::BufferView& jointView     = model.bufferViews[jointAccessor.bufferView];
                    bufferJoints                              = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
                    jointComponentType                        = jointAccessor.componentType;
                    jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) :
                                                                            tinygltf_GetTypeSizeInBytes(jointAccessor);
                }

                if(primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                    const tinygltf::BufferView& weightView     = model.bufferViews[weightAccessor.bufferView];
                    bufferWeights = reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
                    weightByteStride =
                        weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(weightAccessor);
                }

                hasSkin = (bufferJoints && bufferWeights);

                for(size_t v = 0; v < posAccessor.count; v++)
                {
                    Vertex vert{};
                    vert.Pos    = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                    vert.Normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                    vert.Uv0    = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
                    vert.Uv1    = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);

                    if(hasSkin)
                    {
                        switch(jointComponentType)
                        {
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                                const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
                                vert.Joint0         = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
                                break;
                            }
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                                const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
                                vert.Joint0        = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
                                break;
                            }
                            default:
                                throw Exception("Joint component type {} not supported!", jointComponentType);
                        }
                    }
                    else
                    {
                        vert.Joint0 = glm::vec4(0.0f);
                    }
                    vert.Weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
                    // Fix for all zero weights
                    if(glm::length(vert.Weight0) == 0.0f)
                    {
                        vert.Weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                    }
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            if(hasIndices)
            {
                const tinygltf::Accessor&   accessor   = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer&     buffer     = model.buffers[bufferView.buffer];

                indexCount          = static_cast<uint32_t>(accessor.count);
                const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch(accessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        throw Exception("Index component type {} not supported!", accessor.componentType);
                }
            }
            std::unique_ptr<Primitive> newPrimitive =
                std::make_unique<Primitive>(indexStart, indexCount, vertexCount, primitive.material > -1 ? Owner()->Materials()[primitive.material] : Owner()->Materials().back());
            newPrimitive->setBoundingBox(posMin, posMax);
            mPrimitives.push_back(std::move(newPrimitive));
        }
        // Mesh BB from BBs of primitives
        for(auto& p : mPrimitives)
        {
            if(p->bb.valid && !mBoundingBox.valid)
            {
                mBoundingBox       = p->bb;
                mBoundingBox.valid = true;
            }
            mBoundingBox.min = glm::min(mBoundingBox.min, p->bb.min);
            mBoundingBox.max = glm::max(mBoundingBox.max, p->bb.max);
        }
    };

    void Mesh::Cleanup() { uniformBuffer.Buffer.Destroy(); }

    Mesh::~Mesh() { Cleanup(); }

    void Mesh::setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        mBoundingBox.min   = min;
        mBoundingBox.max   = max;
        mBoundingBox.valid = true;
    }


}  // namespace hsk