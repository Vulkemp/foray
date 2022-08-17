#include "hsk_scene.hpp"
#include "globalcomponents/hsk_drawdirector.hpp"
#include "globalcomponents/hsk_geometrystore.hpp"
#include "globalcomponents/hsk_materialbuffer.hpp"
#include "globalcomponents/hsk_texturestore.hpp"
#include "hsk_node.hpp"

namespace hsk {
    Scene::Scene(const VkContext* context) : Registry(&mGlobalRootRegistry), mContext(context)
    {
        InitDefaultGlobals();
    }

    void Scene::InitDefaultGlobals()
    {
        MakeComponent<MaterialBuffer>(mContext);
        MakeComponent<GeometryStore>();
        MakeComponent<TextureStore>();
        MakeComponent<DrawDirector>();
    }

    void Scene::Update(const FrameUpdateInfo& updateInfo)
    {
        this->InvokeUpdate(updateInfo);
        mGlobalRootRegistry.InvokeUpdate(updateInfo);
    }
    void Scene::Draw(const FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)
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

    void Scene::CreateTlas()
    {
        Tlas* tlas = GetComponent<Tlas>();
        if(!tlas)
        {
            tlas = MakeComponent<Tlas>();
        }
        tlas->Create();
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


}  // namespace hsk