#include "minimalraytracingstage.hpp"
#include "../core/samplercollection.hpp"
#include "../core/shadermanager.hpp"
#include "../core/shadermodule.hpp"
#include "../scene/components/meshinstance.hpp"
#include "../scene/scene.hpp"
#include "../scene/globalcomponents/cameramanager.hpp"
#include "../scene/globalcomponents/geometrymanager.hpp"
#include "../scene/globalcomponents/materialmanager.hpp"
#include "../scene/globalcomponents/texturemanager.hpp"
#include "../scene/globalcomponents/tlasmanager.hpp"
#include "../util/pipelinebuilder.hpp"
#include "../util/shaderstagecreateinfos.hpp"
#include <array>

namespace foray::stages {

    MinimalRaytracingStageBase::MinimalRaytracingStageBase(core::Context* context, RenderDomain* domain, int32_t resizeOrder) : RenderStage(context, domain, resizeOrder)
    {
        RenderStage::InitCallbacks(context, domain, resizeOrder);
    }

    void MinimalRaytracingStageBase::DefaultInit()
    {
        ApiCustomObjectsCreate();
        ApiCreateOutputImages();
        ApiCreateOrUpdateDescriptors();
        ApiCreateRtPipeline();
    }

    void MinimalRaytracingStageBase::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        ApiRecordFramePrepare(cmdBuffer, renderInfo);
        ApiRecordFrameBind(cmdBuffer, renderInfo);
        ApiRecordFrameTraceRays(cmdBuffer, renderInfo);
    }
    void MinimalRaytracingStageBase::OnResized(VkExtent2D extent)
    {
        RenderStage::OnResized(extent);
        ApiCreateOrUpdateDescriptors();
    }
    MinimalRaytracingStageBase::~MinimalRaytracingStageBase()
    {
    }
    void MinimalRaytracingStageBase::ReloadShaders()
    {
        ApiDestroyRtPipeline();
        ApiCreateRtPipeline();
    }

}  // namespace foray::stages