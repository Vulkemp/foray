#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "hsk_modelconverter.hpp"

namespace hsk {
    void ModelConverter::BuildGeometryBuffer()
    {
        for(int32_t i = 0; i < mGltfModel.meshes.size(); i++)
        {
            auto&                   gltfMesh = mGltfModel.meshes[i];
            std::vector<Primitive> primitives;
            
            logger()->debug("Model Load: Processing mesh #{} \"{}\" with {} primitives", i, gltfMesh.name, gltfMesh.primitives.size());

            PushGltfMeshToBuffers(gltfMesh, primitives);
            auto mesh = std::make_unique<Mesh>();
            mesh->SetPrimitives(primitives);
            mIndexBindings.Meshes[i] = mesh.get();
            mGeo.GetMeshes().push_back(std::move(mesh));
        }

        mGeo.GetBufferSets().push_back(std::make_unique<GeometryBufferSet>());
        auto geoBufferSet = mGeo.GetBufferSets().back().get();
        geoBufferSet->Init(mContext, mVertexBuffer, mIndexBuffer);

        for(auto& mesh : mIndexBindings.Meshes)
        {
            mesh->SetGeometryBufferSet(geoBufferSet);
            // mesh->BuildAccelerationStructure(mContext);
        }
    }

    void ModelConverter::PushGltfMeshToBuffers(const tinygltf::Mesh& mesh, std::vector<Primitive>& outprimitives)
    {
        outprimitives.resize(mesh.primitives.size());

        for(int32_t i = 0; i < mesh.primitives.size(); i++)
        {
            uint32_t vertexStart = static_cast<uint32_t>(mVertexBuffer.size());
            uint32_t indexStart  = static_cast<uint32_t>(mIndexBuffer.size());

            auto& gltfPrimitive = mesh.primitives[i];
            auto& primitive     = outprimitives[i];

            const std::string POSITION = "POSITION";
            const std::string NORMAL   = "NORMAL";
            const std::string TANGENT  = "TANGENT";
            const std::string TEXCOORD = "TEXCOORD_0";

            auto positionAccessorQuery = gltfPrimitive.attributes.find(POSITION);
            auto normalAccessorQuery   = gltfPrimitive.attributes.find(NORMAL);
            auto tangentAccessorQuery  = gltfPrimitive.attributes.find(TANGENT);
            auto uvAccessorQuery       = gltfPrimitive.attributes.find(TEXCOORD);
            auto failedQuery           = gltfPrimitive.attributes.cend();

            int32_t vertexCount = 0;

            std::function<glm::vec3(int32_t index)> lGetPosition = nullptr;
            std::function<glm::vec3(int32_t index)> lGetNormal   = nullptr;
            std::function<glm::vec3(int32_t index)> lGetTangent  = nullptr;
            std::function<glm::vec2(int32_t index)> lGetUv       = nullptr;
            if(positionAccessorQuery != failedQuery)
            {
                auto& accessor = mGltfModel.accessors[positionAccessorQuery->second];  // The glTF accessor describes in which buffer to look and how to interprete the data stored
                auto& bufferView = mGltfModel.bufferViews[accessor.bufferView];        // The bufferView is a section of a buffer

                vertexCount = static_cast<uint32_t>(accessor.count);

                // Calculate a pointer to the beginning of the relevant buffer
                const float* buffer = reinterpret_cast<const float*>(&(mGltfModel.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
                // Type of the component in bytes
                int32_t typeSize = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(accessor.componentType)) * tinygltf::GetNumComponentsInType(accessor.type);

                // Stride of the data (distance in bytes between each value)
                int32_t byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : typeSize;

                // Set the local function to a lambda
                lGetPosition = [buffer, byteStride](int32_t index) { return glm::make_vec3(&buffer[index * byteStride]); };
            }
            if(normalAccessorQuery != failedQuery)
            {
                auto&        accessor   = mGltfModel.accessors[normalAccessorQuery->second];
                auto&        bufferView = mGltfModel.bufferViews[accessor.bufferView];
                const float* buffer     = reinterpret_cast<const float*>(&(mGltfModel.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
                int32_t      typeSize   = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(accessor.componentType)) * tinygltf::GetNumComponentsInType(accessor.type);
                int32_t      byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : typeSize;

                lGetNormal = [buffer, byteStride](int32_t index) { return glm::make_vec3(&buffer[index * byteStride]); };
            }
            if(tangentAccessorQuery != failedQuery)
            {
                auto&        accessor   = mGltfModel.accessors[tangentAccessorQuery->second];
                auto&        bufferView = mGltfModel.bufferViews[accessor.bufferView];
                const float* buffer     = reinterpret_cast<const float*>(&(mGltfModel.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
                int32_t      typeSize   = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(accessor.componentType)) * tinygltf::GetNumComponentsInType(accessor.type);
                int32_t      byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : typeSize;

                lGetTangent = [buffer, byteStride](int32_t index) { return glm::make_vec3(&buffer[index * byteStride]); };
            }
            if(uvAccessorQuery != failedQuery)
            {
                auto&        accessor   = mGltfModel.accessors[uvAccessorQuery->second];
                auto&        bufferView = mGltfModel.bufferViews[accessor.bufferView];
                const float* buffer     = reinterpret_cast<const float*>(&(mGltfModel.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
                int32_t      typeSize   = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(accessor.componentType)) * tinygltf::GetNumComponentsInType(accessor.type);
                int32_t      byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : typeSize;

                lGetUv = [buffer, byteStride](int32_t index) { return glm::make_vec2(&buffer[index * byteStride]); };
            }

            for(int32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
            {
                mVertexBuffer.push_back(Vertex{.Pos           = lGetPosition ? lGetPosition(vertexIndex) : glm::vec3(),
                                                .Normal        = lGetNormal ? lGetNormal(vertexIndex) : glm::vec3(0.f, 1.f, 0.f),
                                                .Tangent       = lGetTangent ? lGetTangent(vertexIndex) : glm::vec3(0.f, 0.f, 1.f),
                                                .Uv            = lGetUv ? lGetUv(vertexIndex) : glm::vec2(),
                                                .MaterialIndex = gltfPrimitive.material + mIndexBindings.MaterialBufferOffset});
            }

            if(gltfPrimitive.indices >= 0)
            {
                auto& accessor   = mGltfModel.accessors[gltfPrimitive.indices > -1 ? gltfPrimitive.indices : 0];
                auto& bufferView = mGltfModel.bufferViews[accessor.bufferView];
                auto& buffer     = mGltfModel.buffers[bufferView.buffer];

                int32_t indexCount = (int32_t)accessor.count;

                const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch(accessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            mIndexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            mIndexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                        for(size_t index = 0; index < accessor.count; index++)
                        {
                            mIndexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        HSK_THROWFMT("Index component type {} not supported!", accessor.componentType);
                }

                primitive = Primitive(Primitive::EType::Index, indexStart, indexCount);
            }
            else
            {
                primitive = Primitive(Primitive::EType::Vertex, vertexStart, vertexCount);
            }
        }
    }
}  // namespace hsk