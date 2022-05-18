#include "hsk_rasterizedRenderStage.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void RasterizedRenderStage::Destroy() {
        DestroyResolutionDependentComponents();
        DestroyFixedComponents();
        RenderStage::Destroy();
    }

    void RasterizedRenderStage::DestroyFixedComponents(){

    }
    void RasterizedRenderStage::DestroyResolutionDependentComponents() {

    }
}  // namespace hsk