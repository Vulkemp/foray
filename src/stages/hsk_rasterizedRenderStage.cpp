#include "hsk_rasterizedRenderStage.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void RasterizedRenderStage::InitDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) {
        VkDescriptorPoolCreateInfo descriptorPoolCI{};
        descriptorPoolCI.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        // descriptorPoolCI.pNext;
        // descriptorPoolCI.flags;
        descriptorPoolCI.maxSets       = maxSets;
        descriptorPoolCI.poolSizeCount = poolSizes.size();
        descriptorPoolCI.pPoolSizes    = poolSizes.data();

        HSK_ASSERT_VKRESULT(vkCreateDescriptorPool(mContext->Device, &descriptorPoolCI, nullptr, &mDescriptorPool));
    }
    void RasterizedRenderStage::Destroy() {
        if(mDescriptorPool)
        {
            vkDestroyDescriptorPool(mContext->Device, mDescriptorPool, nullptr);
        }
        DestroyResolutionDependentComponents();
        DestroyFixedComponents();
        RenderStage::Destroy();
    }

    void RasterizedRenderStage::DestroyFixedComponents(){

    }
    void RasterizedRenderStage::DestroyResolutionDependentComponents() {

    }
}  // namespace hsk