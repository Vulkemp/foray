#include "hsk_descriptorset.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    void DescriptorSet::SetDescriptorInfoAt(uint32_t set, uint32_t binding, std::shared_ptr<DescriptorInfo> descriptorSetInfo)
    {
        mDescriptorLocations.push_back({set, binding, descriptorSetInfo});
    }

    VkDescriptorSetLayout DescriptorSet::Create(const VkContext* context, uint32_t numSets)
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
            poolSize.descriptorCount = descriptorLocation.DescriptorInfos->NumberOfDescriptors;
                                           

            // prepare pool size
            poolSizes.push_back(poolSize);
        }

        // --------------------------------------------------------------------------------------------
        // allocate descriptor pool

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = static_cast<uint32_t>(numSets);

        AssertVkResult(vkCreateDescriptorPool(context->Device, &poolInfo, nullptr, &mDescriptorPool));

        // --------------------------------------------------------------------------------------------
        // allocate descriptor sets
        std::vector<VkDescriptorSetLayout> layouts(numSets, mDescriptorSetLayout);
        VkDescriptorSetAllocateInfo        descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool     = mDescriptorPool;
        descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(numSets);
        descriptorSetAllocInfo.pSetLayouts        = layouts.data();

        mDescriptorSets.resize(numSets);
        AssertVkResult(vkAllocateDescriptorSets(context->Device, &descriptorSetAllocInfo, mDescriptorSets.data()));


        // TODO: figure out how desciptor set updates work ..
        for(size_t i = 0; i < numSwapchainImages; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mCameraUbos[i];
            bufferInfo.offset = 0;
            bufferInfo.range  = sizeof(CameraUbo);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet           = mDescriptorSets[i];
            descriptorWrite.dstBinding       = 0;
            descriptorWrite.dstArrayElement  = 0;
            descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount  = 1;
            descriptorWrite.pBufferInfo      = &bufferInfo;
            descriptorWrite.pImageInfo       = nullptr;  // Optional
            descriptorWrite.pTexelBufferView = nullptr;  // Optional
            vkUpdateDescriptorSets(context->Device, 1, &descriptorWrite, 0, nullptr);
        }

    }
}  // namespace hsk
