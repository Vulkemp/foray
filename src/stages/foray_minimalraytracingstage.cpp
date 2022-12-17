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

    void MinimalRaytracingStageBase::Init(core::Context* context)
    {
        Destroy();
        mContext = context;
        ApiCustomObjectsCreate();
        ApiCreateOutputImages();
        ApiCreateOrUpdateDescriptors();
        ApiCreatePipelineLayout();
        ApiCreateRtPipeline();
    }
    void MinimalRaytracingStageBase::RecordFrame(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo)
    {
        ApiRecordFramePrepare(cmdBuffer, renderInfo);
        ApiRecordFrameBind(cmdBuffer, renderInfo);
        ApiRecordFrameTraceRays(cmdBuffer, renderInfo);
    }
    void MinimalRaytracingStageBase::Resize(const VkExtent2D& extent)
    {
        RenderStage::Resize(extent);
        ApiCreateOrUpdateDescriptors();
    }
    void MinimalRaytracingStageBase::Destroy()
    {
        ApiDestroyRtPipeline();
        mPipelineLayout.Destroy();
        ApiDestroyDescriptors();
        DestroyOutputImages();
        ApiCustomObjectsDestroy();
    }
    void MinimalRaytracingStageBase::ReloadShaders()
    {
        ApiDestroyRtPipeline();
        ApiCreateRtPipeline();
    }

}  // namespace foray::stages