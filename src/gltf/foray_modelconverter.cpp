#include "foray_modelconverter.hpp"
#include "../core/foray_context.hpp"
#include "../foray_glm.hpp"
#include "../foray_logger.hpp"
#include "../osi/foray_env.hpp"
#include "../scene/components/foray_node_components.hpp"
#include "../scene/globalcomponents/foray_global_components.hpp"
#include <filesystem>

namespace foray::gltf {
    ModelConverter::ModelConverter(scene::Scene* scene)
        : mScene(scene)
        , mMaterialBuffer(*(scene->GetComponent<scene::gcomp::MaterialManager>()))
        , mGeo(*(scene->GetComponent<scene::gcomp::GeometryStore>()))
        , mTextures(*(scene->GetComponent<scene::gcomp::TextureManager>()))
        , mBenchmark(false)
    {
    }

    void ModelConverter::LoadGltfModel(osi::Utf8Path utf8Path, core::Context* context, const ModelConverterOptions& options)
    {
        mBenchmark.Start();
        mContext = context ? context : mScene->GetContext();
        mOptions = options;
        tinygltf::TinyGLTF gltfContext;

        std::string error;
        std::string warning;

        bool binary = false;

        if (utf8Path.IsRelative())
        {
            utf8Path = utf8Path.MakeAbsolute();
        }

        {
            using namespace std::filesystem;

            path p = utf8Path;
            if(!p.has_filename() || !exists(p))
            {
                FORAY_THROWFMT("ModelLoad: Failed because path \"{}\" is not a file!", utf8Path)
            }

            std::string ext = osi::ToUtf8Path(p.extension());
            binary          = ext == ".glb";

            mUtf8Dir = osi::ToUtf8Path(p.parent_path());
        }

        logger()->info("Model Load: Loading tinygltf model ...");


        bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&mGltfModel, &error, &warning, utf8Path) :
                                   gltfContext.LoadASCIIFromFile(&mGltfModel, &error, &warning, utf8Path);

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

        mBenchmark.LogTimestamp("tinyGltf");

        logger()->info("Model Load: Preparing scene buffers ...");

        mScene->GetNodeBuffer().reserve(mGltfModel.nodes.size());
        mIndexBindings.Nodes.resize(mGltfModel.nodes.size());

        mIndexBindings.MaterialBufferOffset = mMaterialBuffer.GetVector().size();
        mMaterialBuffer.GetVector().resize(mIndexBindings.MaterialBufferOffset + mGltfModel.materials.size());

        mIndexBindings.TextureBufferOffset = mTextures.GetTextures().size();
        mTextures.GetTextures().reserve(mGltfModel.textures.size());

        mGeo.GetMeshes().reserve(mGltfModel.meshes.size());
        mIndexBuffer                    = &mGeo.GetIndices();
        mVertexBuffer                   = &mGeo.GetVertices();
        mIndexBindings.IndexBufferStart = mIndexBuffer->size();

        mIndexBindings.Meshes.resize(mGltfModel.meshes.size());
        std::vector<scene::Node*> nodesWithMeshInstances{};
        mScene->FindNodesWithComponent<scene::ncomp::MeshInstance>(nodesWithMeshInstances);
        mNextMeshInstanceIndex = 0;
        if(nodesWithMeshInstances.size())
        {
            for(scene::Node* node : nodesWithMeshInstances)
            {
                mNextMeshInstanceIndex = std::max(mNextMeshInstanceIndex, node->GetComponent<scene::ncomp::MeshInstance>()->GetInstanceIndex());
            }
            mNextMeshInstanceIndex++;
        }


        if(!!mOptions.SceneSelect)
        {
            mGltfScene = &(mGltfModel.scenes[mOptions.SceneSelect(mGltfModel)]);
        }
        else
        {
            mGltfScene = &(mGltfModel.scenes[mGltfModel.defaultScene]);
        }

        logger()->info("Model Load: Building vertex and index buffers ...");

        BuildGeometryBuffer();

        mBenchmark.LogTimestamp("Geometry");

        logger()->info("Model Load: Uploading textures ...");

        LoadTextures();

        mBenchmark.LogTimestamp("Textures");

        logger()->info("Model Load: Uploading materials ...");

        LoadMaterials();


        mBenchmark.LogTimestamp("Materials");

        logger()->info("Model Load: Initialising scene state ...");

        for(int32_t nodeIndex : mGltfScene->nodes)
        {
            RecursivelyTranslateNodes(nodeIndex, nullptr);
        }

        mBenchmark.LogTimestamp("Nodes");

        logger()->info("Model Load: Loading Animations ...");

        LoadAnimations();

        DetectAnimatedNodes();

        mBenchmark.LogTimestamp("Animations");

        InitialUpdate();

        mBenchmark.LogTimestamp("Init");

        Reset();

        mBenchmark.Finalize("Reset");

        logger()->info("Model Load: Done");
    }

    void ModelConverter::RecursivelyTranslateNodes(int32_t currentIndex, scene::Node* parent)
    {
        scene::Node*& node = mIndexBindings.Nodes[currentIndex];

        if(node)
        {
            return;
        }

        auto& gltfNode = mGltfModel.nodes[currentIndex];
        node           = mScene->MakeNode(parent);
        node->SetName(gltfNode.name);

        InitTransformFromGltf(node->GetTransform(), gltfNode.matrix, gltfNode.translation, gltfNode.rotation, gltfNode.scale);

        if(gltfNode.mesh >= 0)
        {
            auto meshInstance = node->MakeComponent<scene::ncomp::MeshInstance>();
            meshInstance->SetMesh(mIndexBindings.Meshes[gltfNode.mesh]);
            meshInstance->SetInstanceIndex(mNextMeshInstanceIndex);
            mNextMeshInstanceIndex++;
        }

        auto lightIter = gltfNode.extensions.find("KHR_lights_punctual");
        if (lightIter != gltfNode.extensions.cend())
        {
            int32_t lightIdx = lightIter->second.Get("light").GetNumberAsInt();
            scene::ncomp::PunctualLight* light = node->MakeComponent<scene::ncomp::PunctualLight>();
            TranslateLight(light, mGltfModel.lights[lightIdx]);
        }

        if (gltfNode.camera >= 0)
        {
            scene::ncomp::Camera* camera = node->MakeComponent<scene::ncomp::Camera>();
            TranslateCamera(camera, mGltfModel.cameras[gltfNode.camera]);
        }

        for(int32_t childIndex : gltfNode.children)
        {
            RecursivelyTranslateNodes(childIndex, node);
        }
    }

    void ModelConverter::InitTransformFromGltf(scene::ncomp::Transform*          transform,
                                               const std::vector<double>& matrix,
                                               const std::vector<double>& translation,
                                               const std::vector<double>& rotation,
                                               const std::vector<double>& scale)
    {
        auto& transformMatrix      = transform->GetLocalMatrix();
        auto& transformTranslation = transform->GetTranslation();
        auto& transformRotation    = transform->GetRotation();
        auto& transformScale       = transform->GetScale();
        if(matrix.size() > 0)
        {
            FORAY_ASSERTFMT(matrix.size() == 16, "Error loading node. Matrix vector expected to have 16 entries, but has {}", matrix.size())

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
            if(mOptions.FlipY)
            {
                transformMatrix[3][2] = -1.f * transformMatrix[3][2];
            }

            // glm::vec3 scale;
            // glm::quat rotation;
            // glm::vec3 translation;
            // glm::vec3 skew;
            // glm::vec4 perspective;
            // glm::decompose(transformMatrix, transformScale, transformRotation, transformTranslation, skew, perspective);
        }
        if(translation.size() > 0)
        {
            FORAY_ASSERTFMT(translation.size() == 3, "Error loading node. Translation vector expected to have 3 entries, but has {}", translation.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_translation

            for(int32_t i = 0; i < 3; i++)
            {
                transformTranslation[i] = (float)translation[i];
            }
            if(mOptions.FlipY)
            {
                transformTranslation.y = -1.f * transformTranslation.y;
            }
        }
        if(rotation.size() > 0)
        {
            FORAY_ASSERTFMT(rotation.size() == 4, "Error loading node. Rotation vector expected to have 4 entries, but has {}", rotation.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_rotation

            for(int32_t i = 0; i < 4; i++)
            {
                transformRotation[i] = (float)rotation[i];
            }
        }
        if(scale.size() > 0)
        {
            FORAY_ASSERTFMT(scale.size() == 3, "Error loading node. Scale vector expected to have 3 entries, but has {}", scale.size())

            // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_node_scale

            for(int32_t i = 0; i < 3; i++)
            {
                transformScale[i] = (float)scale[i];
            }
        }
        if(!translation.size() && !scale.size() && !rotation.size())
        {
            // Set it static so that the local transform never is updated
            transform->SetLocalMatrixFixed(true);
        }
    }

    void ModelConverter::TranslateLight(scene::ncomp::PunctualLight* component, const tinygltf::Light& light)
    {
        component->SetColor(glm::vec3(light.color[0], light.color[1], light.color[2]));
        component->SetIntensity(light.intensity);
        component->SetType(light.type == "directional" ? scene::ELightType::Directional : scene::ELightType::Point);
        component->SetName(light.name);
    }

    void ModelConverter::TranslateCamera(scene::ncomp::Camera* component, const tinygltf::Camera& camera)
    {
        component->InitDefault();
        component->SetName(camera.name);
    }

    void MarkNodeRecursively(scene::Node* node, bool parentAnimated, std::unordered_set<scene::Node*>& animationTargets)
    {
        bool isAnimated = parentAnimated;
        if(!parentAnimated)
        {
            isAnimated = animationTargets.contains(node);
        }
        bool markStatic = !isAnimated;
        node->GetTransform()->SetStatic(markStatic);
        if(isAnimated)
        {
            node->GetTransform()->SetLocalMatrixFixed(false);
        }
        for(auto child : node->GetChildren())
        {
            MarkNodeRecursively(child, isAnimated, animationTargets);
        }
    }

    void ModelConverter::DetectAnimatedNodes()
    {
        scene::gcomp::AnimationManager* animDirector = mScene->GetComponent<scene::gcomp::AnimationManager>();

        std::unordered_set<scene::Node*> animationTargets;

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
            node->GetTransform()->RecalculateIfDirty(true);
        }

        mScene->GetComponent<scene::gcomp::DrawDirector>()->InitOrUpdate();
        mScene->GetComponent<scene::gcomp::CameraManager>()->RefreshCameraList();

        mMaterialBuffer.UpdateDeviceLocal();
    }

    void ModelConverter::Reset()
    {
        mGltfScene             = nullptr;
        mGltfModel             = tinygltf::Model();
        mIndexBindings         = {};
        mNextMeshInstanceIndex = 0;
        mVertexBuffer          = nullptr;
        mIndexBuffer           = nullptr;
    }
}  // namespace foray::gltf