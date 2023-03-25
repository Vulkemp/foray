#include "foray_scene.hpp"
#include "../osi/foray_osmanager.hpp"
#include "components/foray_node_components.hpp"
#include "foray_node.hpp"
#include "globalcomponents/foray_global_components.hpp"


namespace foray::scene {
    Scene::Scene(core::Context* context) : Registry(this), mContext(context)
    {
        InitDefaultGlobals();
        mOnOsEvent.Set(mContext->OsManager->OnEvent(), [this](const osi::Event* event) { this->HandleEvent(event); });
    }

    void Scene::InitDefaultGlobals()
    {
        MakeComponent<gcomp::MaterialManager>(mContext);
        MakeComponent<gcomp::GeometryStore>();
        MakeComponent<gcomp::TextureManager>();
        MakeComponent<gcomp::DrawDirector>();
        MakeComponent<gcomp::CameraManager>(mContext);
    }

    void Scene::Update(VkCommandBuffer cmdBuffer, const base::FrameRenderInfo& renderInfo, stages::RenderDomain* domain)
    {
        SceneUpdateInfo updateInfo(renderInfo, cmdBuffer, domain->GetExtent());
        this->InvokeUpdate(updateInfo);
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

    void Scene::UseDefaultCamera(bool invertAll)
    {
        Node* cameraNode = MakeNode();

        ncomp::Camera* camera = cameraNode->MakeComponent<foray::scene::ncomp::Camera>();
        camera->InitDefault();
        camera->SetName("Free Flight Camera");
        cameraNode->MakeComponent<ncomp::FreeCameraController>()->SetInvertAll(invertAll);
        gcomp::CameraManager* cameraManager = GetComponent<gcomp::CameraManager>();

        cameraManager->RefreshCameraList();
        cameraManager->SelectCamera(camera);
    }

    void Scene::UpdateTlasManager()
    {
        gcomp::TlasManager* tlasManager = GetComponent<gcomp::TlasManager>();
        if(!tlasManager)
        {
            tlasManager = MakeComponent<gcomp::TlasManager>();
        }
        tlasManager->CreateOrUpdate();
    }

    void Scene::UpdateLightManager()
    {
        gcomp::LightManager* lightManager = GetComponent<gcomp::LightManager>();
        if(!lightManager)
        {
            lightManager = MakeComponent<gcomp::LightManager>();
        }
        lightManager->CreateOrUpdate();
    }

    void Scene::Destroy()
    {
        mOnOsEvent.Destroy();
        
        // Clear Nodes (automatically clears attached components via Node deconstructor, called by the deconstructing unique_ptr)
        mRootNodes.clear();
        mNodeBuffer.clear();

        // Clear global components
        Registry::Destroy();
    }


}  // namespace foray::scene