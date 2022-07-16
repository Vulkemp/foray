#include "hsk_rasterizedRenderStage.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void RasterizedRenderStage::Destroy() { RenderStage::Destroy(); }

    void RasterizedRenderStage::OnResized(const VkExtent2D& extent) { RenderStage::OnResized(extent); }
}  // namespace hsk