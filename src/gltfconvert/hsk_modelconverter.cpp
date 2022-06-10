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

    void ModelConverter::LoadGltfModel(std::string utf8Path, const VkContext* context, std::function<int32_t(tinygltf::Model)> sceneSelect)
    {
        mContext = context ? context : mScene->GetContext();
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

        mIndexBindings.NodeBufferOffset = mScene->GetNodeBuffer().size();
        mScene->GetNodeBuffer().resize(mIndexBindings.NodeBufferOffset + mGltfModel.nodes.size());

        mIndexBindings.MaterialBufferOffset = mMaterialBuffer.GetVector().size();
        mMaterialBuffer.GetVector().resize(mIndexBindings.MaterialBufferOffset + mGltfModel.materials.size());

        mTextures.GetTextures().reserve(mGltfModel.textures.size());
        mIndexBindings.Textures.resize(mGltfModel.textures.size());

        mGeo.GetMeshes().reserve(mGltfModel.meshes.size());
        mIndexBindings.Meshes.resize(mGltfModel.meshes.size());
        mNextMeshInstanceIndex = mTransformBuffer.GetVector().size();

        if(sceneSelect)
        {
            mGltfScene = &(mGltfModel.scenes[sceneSelect(mGltfModel)]);
        }
        else
        {
            mGltfScene = &(mGltfModel.scenes[mGltfModel.defaultScene]);
        }

        BuildGeometryBuffer();
        LoadTextures();

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
        int32_t nodeBufferIndex = mIndexBindings.NodeBufferOffset + currentIndex;
        auto& bufferUniquePtr = mScene->GetNodeBuffer()[nodeBufferIndex];
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
            meshInstance->SetMesh(mIndexBindings.Meshes[gltfNode.mesh]);
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
}  // namespace hsk