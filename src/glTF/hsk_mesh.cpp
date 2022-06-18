#include "hsk_mesh.hpp"
#include "../hsk_vkHelpers.hpp"
#include "hsk_scene.hpp"
#include "hsk_scenedrawinfo.hpp"
#include "../hsk_glm.hpp"

namespace hsk {
    MeshInstance::MeshInstance() {}

    MeshInstance::MeshInstance(Scene* scene) : SceneComponent(scene) {}

    Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material* material)
        : FirstIndex(firstIndex), IndexCount(indexCount), VertexCount(vertexCount), Mat(material)
    {
        HasIndices = indexCount > 0;
    };

    void Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        Bounds.SetMin(min);
        Bounds.SetMax(max);
        Bounds.SetValid(true);
    }

    void MeshInstance::InitFromTinyGltfMesh(
        const tinygltf::Model& model, const tinygltf::Mesh& mesh, int32_t index, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
    {
        // TODO: This function is incredibly ugly

        mIndex = index;
        mPushConstant.MeshId = (int32_t)index;

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
                const float* bufferTangents     = nullptr;
                const float* bufferTexCoordSet0 = nullptr;
                const float* bufferTexCoordSet1 = nullptr;
                // const void*  bufferJoints       = nullptr;
                // const float* bufferWeights      = nullptr;

                int posByteStride;
                int normByteStride;
                int tangentByteStride;
                int uv0ByteStride;
                int uv1ByteStride;
                // int jointByteStride;
                // int weightByteStride;
                // int jointComponentType;

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

                if(primitive.attributes.find("TANGENT") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   tangentAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
                    const tinygltf::BufferView& tangentView     = model.bufferViews[tangentAccessor.bufferView];
                    bufferTangents = reinterpret_cast<const float*>(&(model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
                    tangentByteStride =
                        tangentAccessor.ByteStride(tangentView) ? (tangentAccessor.ByteStride(tangentView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(tangentAccessor);
                }

                if(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor&   uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& uvView     = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet0                     = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf_GetTypeSizeInBytes(uvAccessor);
                }

                uint32_t materialIndex = (uint32_t)primitive.material;

                for(size_t v = 0; v < posAccessor.count; v++)
                {
                    Vertex vert{};
                    vert.Pos           = glm::vec3(glm::make_vec3(&bufferPos[v * posByteStride]));
                    vert.Normal        = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                    vert.Tangent       = glm::normalize(glm::vec3(bufferTangents ? glm::make_vec3(&bufferTangents[v * tangentByteStride]) : glm::vec3(0.0f)));
                    vert.Uv            = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec2(0.0f);
                    vert.MaterialIndex = materialIndex;

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
                        HSK_THROWFMT("Index component type {} not supported!", accessor.componentType);
                }
            }
            std::unique_ptr<Primitive> newPrimitive =
                std::make_unique<Primitive>(indexStart, indexCount, vertexCount, primitive.material > -1 ? &(Owner()->Materials()[primitive.material]) : nullptr);
            newPrimitive->setBoundingBox(posMin, posMax);
            mPrimitives.push_back(std::move(newPrimitive));
        }
        // Mesh BB from BBs of primitives
        for(auto& p : mPrimitives)
        {
            if(p->Bounds.GetValid() && !mBounds.GetValid())
            {
                mBounds            = p->Bounds;
                mBounds.GetValid() = true;
            }
            mBounds.SetMin(glm::min(mBounds.GetMin(), p->Bounds.GetMin()));
            mBounds.SetMax(glm::max(mBounds.GetMax(), p->Bounds.GetMax()));
        }
    };

    void MeshInstance::Cleanup() {}

    MeshInstance::~MeshInstance() { Cleanup(); }

    void MeshInstance::setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        mBounds.SetMin(min);
        mBounds.SetMax(max);
        mBounds.SetValid(true);
    }

    void MeshInstance::Update(const glm::mat4& mat)
    {
        // Todo: update model matrix in SceneTransformState Vector
        auto& sceneTransformState = mScene->GetTransformState();
        auto& modelTransformState = sceneTransformState.Vector()[mIndex];
        modelTransformState.PreviousModelMatrix = modelTransformState.ModelMatrix;
        modelTransformState.ModelMatrix = mat;
    }

    void MeshInstance::Draw(SceneDrawInfo& drawInfo)
    {
        vkCmdPushConstants(drawInfo.CmdBuffer, drawInfo.PipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &mPushConstant);
        for(const auto& primitive : mPrimitives)
        {
            vkCmdDrawIndexed(drawInfo.CmdBuffer, primitive->IndexCount, 1, primitive->FirstIndex, 0, 0);
        }
    }

}  // namespace hsk