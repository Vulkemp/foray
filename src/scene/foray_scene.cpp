#include "foray_scene.hpp"
#include "components/foray_camera.hpp"
#include "components/foray_freecameracontroller.hpp"
#include "foray_node.hpp"
#include "globalcomponents/foray_cameramanager.hpp"
#include "globalcomponents/foray_drawdirector.hpp"
#include "globalcomponents/foray_geometrystore.hpp"
#include "globalcomponents/foray_materialbuffer.hpp"
#include "globalcomponents/foray_texturestore.hpp"
#include "globalcomponents/foray_tlasmanager.hpp"


namespace foray::scene {
    Scene::Scene(core::Context* context) : Registry(this), mContext(context)
    {
        InitDefaultGlobals();
    }

    void Scene::InitDefaultGlobals()
    {
        MakeComponent<MaterialBuffer>(mContext);
        MakeComponent<GeometryStore>();
        MakeComponent<TextureStore>();
        MakeComponent<DrawDirector>();
        MakeComponent<CameraManager>(mContext);
    }

    void Scene::Update(const base::FrameRenderInfo& renderInfo, base::CmdBufferIndex index)
    {
        SceneUpdateInfo updateInfo(renderInfo, index);
        this->InvokeUpdate(updateInfo);
    }
    void Scene::Update(const base::FrameRenderInfo& renderInfo, VkCommandBuffer cmdBuffer)
    {
        SceneUpdateInfo updateInfo(renderInfo, cmdBuffer);
        this->InvokeUpdate(updateInfo);
    }
    void Scene::Draw(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, base::CmdBufferIndex index)
    {
        // Process draw callbacks
        SceneDrawInfo drawInfo(renderInfo, pipelineLayout, index);
        this->InvokeDraw(drawInfo);
    }
    void Scene::Draw(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout, VkCommandBuffer cmdBuffer)
    {
        // Process draw callbacks
        SceneDrawInfo drawInfo(renderInfo, pipelineLayout, cmdBuffer);
        this->InvokeDraw(drawInfo);
    }

    void Scene::HandleEvent(const osi::Event* event)
    {
        this->InvokeOnEvent(event);
    }

    Node* Scene::MakeNode(Node* parent)
    {
        auto nodeManagedPtr = std::make_unique<Node>(this, parent);
        auto node           = nodeManagedPtr.get();
        mNodeBuffer.push_back(std::move(nodeManagedPtr));
        if(!parent)
        {
            mRootNodes.push_back(node);
        }
        else
        {
            parent->GetChildren().push_back(node);
        }
        return node;
    }

    void Scene::UseDefaultCamera()
    {
        Node* cameraNode = MakeNode();

        Camera* camera = cameraNode->MakeComponent<foray::scene::Camera>();
        camera->InitDefault();
        cameraNode->MakeComponent<FreeCameraController>();
        CameraManager* cameraManager = GetComponent<CameraManager>();

        cameraManager->RefreshCameraList();
        cameraManager->SelectCamera(camera);
    }

    void Scene::UpdateTlasManager()
    {
        TlasManager* tlasManager = GetComponent<TlasManager>();
        if(!tlasManager)
        {
            tlasManager = MakeComponent<TlasManager>();
        }
        tlasManager->CreateOrUpdate();
    }

    void Scene::Destroy(bool reinitialize)
    {
        // Clear Nodes (automatically clears attached components via Node deconstructor, called by the deconstructing unique_ptr)
        mRootNodes.clear();
        mNodeBuffer.clear();

        // Clear global components
        Registry::Destroy();
        if(reinitialize)
        {
            InitDefaultGlobals();
        }
    }


}  // namespace foray::scene