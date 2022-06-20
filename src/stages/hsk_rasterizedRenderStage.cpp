#include "hsk_rasterizedRenderStage.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void RasterizedRenderStage::Destroy() {
        DestroyResolutionDependentComponents();
        DestroyFixedComponents();
        RenderStage::Destroy();
    }

    void RasterizedRenderStage::OnResized(const VkExtent2D& extent)
    {
        DestroyResolutionDependentComponents();
        CreateResolutionDependentComponents();
    }
}  // namespace hsk