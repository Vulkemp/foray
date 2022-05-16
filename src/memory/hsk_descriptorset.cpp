#include "hsk_descriptorset.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void DescriptorSet::SetDescriptorInfoAt(uint32_t set, uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorSetInfo)
    {
        mDescriptorLocations.push_back({set, binding, descriptorSetInfo});
    }

    VkDescriptorSetLayout DescriptorSet::Create(const VkContext* context, uint32_t maxSets)
    {

        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding            = 0;
        uboLayoutBinding.descriptorCount    = 1;
        uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(mDescriptorLocations.size());

        for(size_t i = 0; i < mDescriptorLocations.size(); i++)
        {
            auto& descriptorLocation = mDescriptorLocations[i];
            layoutBindings.push_back(descriptorLocation.DescriptorInfos->DescriptorSetLayoutBinding);
            layoutBindings[i].binding = descriptorLocation.Binding;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = mDescriptorLocations.size();
        layoutInfo.pBindings    = layoutBindings.data();

        AssertVkResult(vkCreateDescriptorSetLayout(context->Device, &layoutInfo, nullptr, &mDescriptorSetLayout));

        // descriptor pool
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(mDescriptorLocations.size());

        for(auto& descriptorLocation : mDescriptorLocations)
        {
            if(descriptorLocation.DescriptorInfos->Created)
            {
                // skip creating descriptor sets that are already allocated
                break;
            }

            VkDescriptorPoolSize poolSize{};
            poolSize.type = descriptorLocation.DescriptorInfos->DescriptorSetLayoutBinding.descriptorType;
            poolSize.descriptorCount = descriptorLocation.DescriptorInfos->DescriptorSetLayoutBinding.descriptorCount;

            // prepare pool size
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = static_cast<uint32_t>(maxSets);

        AssertVkResult(vkCreateDescriptorPool(context->Device, &poolInfo, nullptr, &mDescriptorPool));
    }
}  // namespace hsk
