#include "hsk_modelconverter.hpp"
#include "../base/hsk_vkcontext.hpp"
#include "../hsk_env.hpp"
#include "../hsk_glm.hpp"
#include "../scenegraph/components/hsk_meshinstance.hpp"
#include "../scenegraph/components/hsk_transform.hpp"
#include "../scenegraph/globalcomponents/hsk_animationdirector.hpp"
#include "../scenegraph/globalcomponents/hsk_drawdirector.hpp"
#include "../scenegraph/globalcomponents/hsk_geometrystore.hpp"
#include "../scenegraph/globalcomponents/hsk_materialbuffer.hpp"
#include "../scenegraph/globalcomponents/hsk_texturestore.hpp"
#include <filesystem>

namespace hsk {
    ModelConverter::ModelConverter(Scene* scene)
        : mScene(scene), mMaterialBuffer(*(scene->GetComponent<MaterialBuffer>())), mGeo(*(scene->GetComponent<GeometryStore>())), mTextures(*(scene->GetComponent<TextureStore>()))
    {
    }

    void ModelConverter::LoadGltfModel(std::string utf8Path, const VkContext* context, std::function<int32_t(tinygltf::Model)> sceneSelect)
    {
        mContext = context ? context : mScene->GetContext();
        tinygltf::TinyGLTF gltfContext;

        std::string error;
        std::string warning;

        bool binary = false;

        {
            using namespace std::filesystem;

            path p = FromUtf8Path(utf8Path);
            if(!p.has_filename() || !exists(p))
            {
                HSK_THROWFMT("ModelLoad: Failed because path \"{}\" is not a file!", utf8Path)
            }

            std::string ext = ToUtf8Path(p.extension());
            binary          = ext == ".glb";

            mUtf8Dir = ToUtf8Path(p.parent_path());
        }

        logger()->info("Model Load: Loading tinygltf model ...");


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

        logger()->info("Model Load: Preparing scene buffers ...");

        mScene->GetNodeBuffer().reserve(mGltfModel.nodes.size());
        mIndexBindings.Nodes.resize(mGltfModel.nodes.size());

        mIndexBindings.MaterialBufferOffset = mMaterialBuffer.GetVector().size();
        mMaterialBuffer.GetVector().resize(mIndexBindings.MaterialBufferOffset + mGltfModel.materials.size());

        mIndexBindings.TextureBufferOffset = mTextures.GetTextures().size();
        mTextures.GetTextures().reserve(mGltfModel.textures.size());

        mGeo.GetMeshes().reserve(mGltfModel.meshes.size());
        mIndexBindings.Meshes.resize(mGltfModel.meshes.size());
        std::vector<Node*> nodesWithMeshInstances{};
        mScene->FindNodesWithComponent<MeshInstance>(nodesWithMeshInstances);
        mNextMeshInstanceIndex = 0;
        if(nodesWithMeshInstances.size())
        {
            for(Node* node : nodesWithMeshInstances)
            {
                mNextMeshInstanceIndex = std::max(mNextMeshInstanceIndex, node->GetComponent<MeshInstance>()->GetInstanceIndex());
            }
            mNextMeshInstanceIndex++;
        }


        if(sceneSelect)
        {
            mGltfScene = &(mGltfModel.scenes[sceneSelect(mGltfModel)]);
        }
        else
        {
            mGltfScene = &(mGltfModel.scenes[mGltfModel.defaultScene]);
        }

        logger()->info("Model Load: Building vertex and index buffers ...");

        BuildGeometryBuffer();

        logger()->info("Model Load: Uploading textures ...");

        LoadTextures();

        logger()->info("Model Load: Uploading materials ...");

        LoadMaterials();

        logger()->info("Model Load: Initialising scene state ...");

        for(int32_t nodeIndex : mGltfScene->nodes)
        {
            RecursivelyTranslateNodes(nodeIndex, nullptr);
        }

        logger()->info("Model Load: Loading Animations ...");

        LoadAnimations();

        DetectAnimatedNodes();

        InitialUpdate();

        Reset();

        logger()->info("Model Load: Done");
    }

    void ModelConverter::RecursivelyTranslateNodes(int32_t currentIndex, Node* parent)
    {
        Node*& node = mIndexBindings.Nodes[currentIndex];

        if(node)
        {
            return;
        }

        auto& gltfNode = mGltfModel.nodes[currentIndex];
        node           = mScene->MakeNode(parent);

        InitTransformFromGltf(node->GetTransform(), gltfNode.matrix, gltfNode.translation, gltfNode.rotation, gltfNode.scale);

        if(gltfNode.mesh >= 0)
        {
            auto meshInstance = node->MakeComponent<MeshInstance>();
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
        Transform* transform, const std::vector<double>& matrix, const std::vector<double>& translation, const std::vector<double>& rotation, const std::vector<double>& scale)
    {
        auto& transformMatrix      = transform->GetLocalMatrix();
        auto& transformTranslation = transform->GetTranslation();
        auto& transformRotation    = transform->GetRotation();
        auto& transformScale       = transform->GetScale();
        if(matrix.size() > 0)
        {
            HSK_ASSERTFMT(matrix.size() == 16, "Error loading node. Matrix vector expected to have 16 entries, but has {}", matrix.size())

            if(translation.size() == 0 && rotation.size() == 0 && scale.size() == 0)
            {
                // This happens because the recursive matrix recalculation step would immediately overwrite the transform matrix!
                logger()->warn("Node has transform matrix specified, but no transform components. This will prevent writes to local matrix!");
            }

            // GLM and gltf::node.matrix both are column major, so this is valid:
            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_matrix

            for(int32_t i = 0; i < 16; i++)
            {
                transformMatrix[i / 4][i % 4] = (float)matrix[i];
            }
            transformMatrix[3][2] = -1.f * transformMatrix[3][2];

            // glm::vec3 scale;
            // glm::quat rotation;
            // glm::vec3 translation;
            // glm::vec3 skew;
            // glm::vec4 perspective;
            // glm::decompose(transformMatrix, transformScale, transformRotation, transformTranslation, skew, perspective);
        }
        if(translation.size() > 0)
        {
            HSK_ASSERTFMT(translation.size() == 3, "Error loading node. Translation vector expected to have 3 entries, but has {}", translation.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_translation

            for(int32_t i = 0; i < 3; i++)
            {
                transformTranslation[i] = (float)translation[i];
            }
            transformTranslation.y = -1.f * transformTranslation.y;
        }
        if(rotation.size() > 0)
        {
            HSK_ASSERTFMT(rotation.size() == 4, "Error loading node. Rotation vector expected to have 4 entries, but has {}", rotation.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_rotation

            for(int32_t i = 0; i < 4; i++)
            {
                transformRotation[i] = (float)rotation[i];
            }
        }
        if(scale.size() > 0)
        {
            HSK_ASSERTFMT(scale.size() == 3, "Error loading node. Scale vector expected to have 3 entries, but has {}", scale.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_scale

            for(int32_t i = 0; i < 3; i++)
            {
                transformScale[i] = (float)scale[i];
            }
        }
        if(!translation.size() && !scale.size() && !rotation.size())
        {
            // Set it static so that the local transform never is updated
            transform->SetStatic(true);
        }
    }

    void MarkNodeRecursively(Node* node, bool parentAnimated, std::unordered_set<Node*>& animationTargets)
    {
        bool isAnimated = parentAnimated;
        if(!parentAnimated)
        {
            isAnimated = animationTargets.contains(node);
        }
        bool markStatic = !isAnimated;
        node->GetTransform()->SetStatic(markStatic);
        for(auto child : node->GetChildren())
        {
            MarkNodeRecursively(child, isAnimated, animationTargets);
        }
    }

    void ModelConverter::DetectAnimatedNodes()
    {
        AnimationDirector* animDirector = mScene->GetComponent<AnimationDirector>();

        std::unordered_set<Node*> animationTargets;

        if(!!animDirector)
        {
            for(auto animation : animDirector->GetAnimations())
            {
                for(auto channel : animation.GetChannels())
                {
                    animationTargets.emplace(channel.Target);
                }
            }
        }

        for(auto root : mScene->GetRootNodes())
        {
            MarkNodeRecursively(root, false, animationTargets);
        }
    }

    void ModelConverter::InitialUpdate()
    {
        for(auto node : mScene->GetRootNodes())
        {
            node->GetTransform()->RecalculateGlobalMatrix(nullptr);
        }

        mScene->GetComponent<DrawDirector>()->InitOrUpdate();

        mMaterialBuffer.UpdateDeviceLocal();
    }

    void ModelConverter::Reset()
    {
        mGltfScene             = nullptr;
        mGltfModel             = tinygltf::Model();
        mIndexBindings         = {};
        mNextMeshInstanceIndex = 0;
        mVertexBuffer.clear();
        mIndexBuffer.clear();
    }
}  // namespace hsk