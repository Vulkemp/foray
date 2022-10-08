#include "foray_rasterizedRenderStage.hpp"

namespace foray::stages {
    void RasterizedRenderStage::Destroy() { RenderStage::Destroy(); }

    void RasterizedRenderStage::OnResized(const VkExtent2D& extent) { RenderStage::OnResized(extent); }
}  // namespace foray