#include "foray_minimalraytracingstage.hpp"
#include "../core/foray_samplercollection.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"
#include "../scene/components/foray_meshinstance.hpp"
#include "../scene/foray_scene.hpp"
#include "../scene/globalcomponents/foray_cameramanager.hpp"
#include "../scene/globalcomponents/foray_geometrymanager.hpp"
#include "../scene/globalcomponents/foray_materialmanager.hpp"
#include "../scene/globalcomponents/foray_texturemanager.hpp"
#include "../scene/globalcomponents/foray_tlasmanager.hpp"
#include "../util/foray_pipelinebuilder.hpp"
#include "../util/foray_shaderstagecreateinfos.hpp"
#include <array>

namespace foray::stages {

    MinimalRaytracingStageBase::MinimalRaytracingStageBase(core::Context* context, RenderDomain* domain, int32_t resizeOrder)
    {
        RenderStage::InitCallbacks(context, domain, resizeOrder);
        ApiCustomObjectsCreate();
        ApiCreateOutputImages();
        ApiCreateOrUpdateDescriptors();
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
        mPipelineLayout.Destroy();
    }
    void MinimalRaytracingStageBase::ReloadShaders()
    {
        ApiDestroyRtPipeline();
        ApiCreateRtPipeline();
    }

}  // namespace foray::stages