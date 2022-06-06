#include "hsk_scene.hpp"
#include "globalcomponents/hsk_geometrystore.hpp"
#include "globalcomponents/hsk_materialbuffer.hpp"
#include "globalcomponents/hsk_scenetransformbuffer.hpp"
#include "hsk_node.hpp"

namespace hsk {
    NScene::NScene(const VkContext* context) : Registry(&mGlobalRootRegistry), mContext(context)
    {
        MakeComponent<NMaterialBuffer>(mContext);
        MakeComponent<SceneTransformBuffer>(mContext);
        MakeComponent<GeometryStore>();
    }

    void NScene::Update(const FrameUpdateInfo& updateInfo)
    {
        this->InvokeUpdate(updateInfo);
        mGlobalRootRegistry.InvokeUpdate(updateInfo);
    }
    void NScene::Draw(const FrameRenderInfo& renderInfo, VkPipelineLayout pipelineLayout)
    {
        // Process before draw callbacks
        this->InvokeBeforeDraw(renderInfo);
        mGlobalRootRegistry.InvokeBeforeDraw(renderInfo);

        // Process draw callbacks
        SceneDrawInfo drawInfo(renderInfo, pipelineLayout);
        this->InvokeDraw(drawInfo);
        mGlobalRootRegistry.InvokeDraw(drawInfo);
    }

    void NScene::HandleEvent(std::shared_ptr<Event>& event)
    {
        this->InvokeOnEvent(event);
        mGlobalRootRegistry.InvokeOnEvent(event);
    }

    NNode* NScene::MakeNode(NNode* parent)
    {
        auto nodeManagedPtr = std::make_unique<NNode>(this, parent);
        auto node           = nodeManagedPtr.get();
        mNodeBuffer.push_back(std::move(nodeManagedPtr));
        if(!parent)
        {
            mRootNodes.push_back(node);
        }
        return node;
    }

    void NScene::Cleanup()
    {
        // Clear Nodes (automatically clears attached components via Node deconstructor, called by the deconstructing unique_ptr)
        mRootNodes.clear();
        mNodeBuffer.clear();

        // Clear global components
        Registry::Cleanup();
    }


}  // namespace hsk