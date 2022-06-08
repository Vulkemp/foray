#include "hsk_modelconverter.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/components/hsk_transform.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/globalcomponents/hsk_materialbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_scenetransformbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_texturestore.hpp"
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace hsk {
    ModelConverter::ModelConverter(NScene* scene)
        : mScene(scene)
        , mMaterialBuffer(*(scene->GetComponent<NMaterialBuffer>()))
        , mGeo(*(scene->GetComponent<GeometryStore>()))
        , mTextures(*(scene->GetComponent<TextureStore>()))
        , mTransformBuffer(*(scene->GetComponent<SceneTransformBuffer>()))
    {
    }

    void ModelConverter::LoadGltfModel(const VkContext* context, std::string utf8Path, std::function<int32_t(tinygltf::Model)> sceneSelect)
    {
        tinygltf::TinyGLTF gltfContext;
        std::string        error;
        std::string        warning;

        bool   binary = false;
        size_t extpos = utf8Path.rfind('.', utf8Path.length());
        if(extpos != std::string::npos)
        {
            binary = (utf8Path.substr(extpos + 1, utf8Path.length() - extpos) == "glb");
        }

        bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&mGltfModel, &error, &warning, utf8Path.c_str()) :
                                   gltfContext.LoadASCIIFromFile(&mGltfModel, &error, &warning, utf8Path.c_str());

        if(warning.size())
        {
            logger()->warn("tinygltf warning loading file \"{}\": \"{}\"", utf8Path, warning);
        }
        if(!fileLoaded)
        {
            if(!error.size())
            {
                error = "Unknown error";
            }
            logger()->error("tinygltf error loading file \"{}\": \"{}\"", utf8Path, error);
            Exception::Throw("Failed to load file");
        }

        mScene->Cleanup();

        mScene->GetNodeBuffer().resize(mGltfModel.nodes.size());
        mMaterialBuffer.GetVector().resize(mGltfModel.materials.size());
        mTextures.GetTextures().resize(mGltfModel.textures.size());
        mGeo.GetMeshes().resize(mGltfModel.meshes.size());

        if(sceneSelect)
        {
            mGltfScene = &(mGltfModel.scenes[sceneSelect(mGltfModel)]);
        }
        else
        {
            mGltfScene = &(mGltfModel.scenes[mGltfModel.defaultScene]);
        }

        BuildGeometryBuffer();

        // Discover and load
        for(int32_t nodeIndex : mGltfScene->nodes)
        {
            RecursivelyTranslateNodes(nodeIndex, nullptr);
        }

        for(auto node : mScene->GetRootNodes())
        {
            node->GetTransform()->RecalculateGlobalMatrix(nullptr);
        }
    }

    void ModelConverter::RecursivelyTranslateNodes(int32_t currentIndex, NNode* parent)
    {
        auto& bufferUniquePtr = mScene->GetNodeBuffer()[currentIndex];
        if(bufferUniquePtr)
        {
            return;
        }
        auto& gltfNode  = mGltfModel.nodes[currentIndex];
        bufferUniquePtr = std::make_unique<NNode>(mScene, parent);
        auto node       = bufferUniquePtr.get();
        if(!parent)
        {
            mScene->GetRootNodes().push_back(node);
        }

        InitTransformFromGltf(node->GetTransform(), gltfNode.matrix, gltfNode.translation, gltfNode.rotation, gltfNode.scale);

        if(gltfNode.mesh >= 0)
        {
            auto meshInstance = node->MakeComponent<NMeshInstance>();
            meshInstance->SetMeshIndex(gltfNode.mesh);
            meshInstance->SetInstanceIndex(mNextMeshInstanceIndex);
            mNextMeshInstanceIndex++;
        }

        for(int32_t childIndex : gltfNode.children)
        {
            RecursivelyTranslateNodes(childIndex, node);
        }
    }

    void ModelConverter::InitTransformFromGltf(
        NTransform* transform, const std::vector<double>& matrix, const std::vector<double>& translation, const std::vector<double>& rotation, const std::vector<double>& scale)
    {
        if(matrix.size() > 0)
        {
            HSK_ASSERTFMT(matrix.size() == 16, "Error loading node. Matrix vector expected to have 16 entries, but has {}", matrix.size())

            if(translation.size() == 0 && rotation.size() == 0 && scale.size() == 0)
            {
                // This happens because the recursive matrix recalculation step would immediately overwrite the transform matrix!
                logger()->warn("Node has transform matrix specified, but no transform components. Ignoring matrix!");
                return;
            }

            // GLM and gltf::node.matrix both are column major, so this is valid:
            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_matrix

            for(int32_t i = 0; i < 16; i++)
            {

                auto& transformMatrix         = transform->GetLocalMatrix();
                transformMatrix[i / 4][i % 4] = (float)matrix[i];
            }
        }
        if(translation.size() > 0)
        {
            HSK_ASSERTFMT(translation.size() == 3, "Error loading node. Translation vector expected to have 3 entries, but has {}", translation.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_translation

            for(int32_t i = 0; i < 3; i++)
            {
                auto& transformTranslation = transform->GetTranslation();
                transformTranslation[i]    = (float)translation[i];
            }
        }
        if(rotation.size() > 0)
        {
            HSK_ASSERTFMT(rotation.size() == 4, "Error loading node. Rotation vector expected to have 4 entries, but has {}", rotation.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_rotation

            for(int32_t i = 0; i < 4; i++)
            {
                auto& transformRotation = transform->GetRotation();
                transformRotation[i]    = (float)rotation[i];
            }
        }
        if(scale.size() > 0)
        {
            HSK_ASSERTFMT(scale.size() == 3, "Error loading node. Scale vector expected to have 3 entries, but has {}", scale.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_scale

            for(int32_t i = 0; i < 3; i++)
            {
                auto& transformScale = transform->GetScale();
                transformScale[i]    = (float)scale[i];
            }
        }
    }

    void ModelConverter::BuildGeometryBuffer()
    {
        for(int32_t i = 0; i < mGltfModel.meshes.size(); i++)
        {
            auto&                   gltfMesh = mGltfModel.meshes[i];
            std::vector<NPrimitive> primitives;
            PushGltfMeshToBuffers(gltfMesh, primitives);
            mGeo.GetMeshes()[i] = std::make_unique<Mesh>();
            mGeo.GetMeshes()[i].get()->SetPrimitives(primitives);
        }

        mGeo.GetBufferSets().push_back(std::make_unique<GeometryBufferSet>());
        auto geoBufferSet = mGeo.GetBufferSets().front().get();

        for(auto& mesh : mGeo.GetMeshes())
        {
            mesh->SetBuffer(geoBufferSet);
        }
    }

    void ModelConverter::PushGltfMeshToBuffers(const tinygltf::Mesh& mesh, std::vector<NPrimitive>& outprimitives)
    {
        outprimitives.resize(mesh.primitives.size());

        uint32_t vertexStart = static_cast<uint32_t>(mVertexBuffer.size());
        uint32_t indexStart  = static_cast<uint32_t>(mIndexBuffer.size());

        for(int32_t i = 0; i < mesh.primitives.size(); i++)
        {
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
                mVertexBuffer.push_back(NVertex{.Pos           = lGetPosition ? lGetPosition(vertexIndex) : glm::vec3(),
                                                .Normal        = lGetNormal ? lGetNormal(vertexIndex) : glm::vec3(0.f, 1.f, 0.f),
                                                .Tangent       = lGetTangent ? lGetTangent(vertexIndex) : glm::vec3(0.f, 0.f, 1.f),
                                                .Uv            = lGetUv ? lGetUv(vertexIndex) : glm::vec2(),
                                                .MaterialIndex = gltfPrimitive.material});
            }

            if(gltfPrimitive.indices >= 0)
            {
                auto& accessor   = mGltfModel.accessors[gltfPrimitive.indices];
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

                primitive = NPrimitive(NPrimitive::EType::Index, indexStart, indexCount);
            }
            else
            {
                primitive = NPrimitive(NPrimitive::EType::Vertex, vertexStart, vertexCount);
            }
        }
    }
}  // namespace hsk