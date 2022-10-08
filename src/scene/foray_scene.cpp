#include "foray_scene.hpp"
#include "globalcomponents/foray_cameramanager.hpp"
#include "globalcomponents/foray_drawdirector.hpp"
#include "globalcomponents/foray_geometrystore.hpp"
#include "globalcomponents/foray_materialbuffer.hpp"
#include "globalcomponents/foray_texturestore.hpp"
#include "foray_node.hpp"

namespace foray::scene {
    Scene::Scene(const core::VkContext* context) : Registry(&mGlobalRootRegistry), mContext(context)
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

    void Scene::Update(const base::FrameUpdateInfo& updateInfo)
    {
        this->InvokeUpdate(updateInfo);
        mGlobalRootRegistry.InvokeUpdate(updateInfo);
    }
    void Scene::Draw(const base::FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)
    {
        // Process before draw callbacks
        this->InvokeBeforeDraw(renderInfo);
        mGlobalRootRegistry.InvokeBeforeDraw(renderInfo);

        // Process draw callbacks
        SceneDrawInfo drawInfo(renderInfo, pipelineLayout);
        this->InvokeDraw(drawInfo);
        mGlobalRootRegistry.InvokeDraw(drawInfo);
    }

    void Scene::HandleEvent(const Event* event)
    {
        this->InvokeOnEvent(event);
        mGlobalRootRegistry.InvokeOnEvent(event);
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


}  // namespace foray