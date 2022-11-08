#include "foray_descriptorset.hpp"

namespace foray::core {

    void DescriptorSet::Create(Context* context, std::string debugName, VkDescriptorSetLayout predefinedLayout, VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags)
    {
        mContext = context;
        mName    = debugName;

        if(predefinedLayout)
        {
            mDescriptorSetLayout = predefinedLayout;
            mExternalLayout = true;
        }
        else
        {
            CreateDescriptorSetLayout(descriptorSetLayoutCreateFlags);
        }
        CreateDescriptorSet();
    }

    void DescriptorSet::Update()
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        for(const auto& pairBindingDescriptorInfo : mMapBindingToDescriptorInfo)
        {
            // get binding and corresponding info
            uint32_t              binding        = pairBindingDescriptorInfo.first;
            const DescriptorInfo& descriptorInfo = pairBindingDescriptorInfo.second;

            if (descriptorInfo.DescriptorCount == 0)
            {
                continue;
            }

            // prepare write
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext            = nullptr;
            descriptorWrite.dstSet           = mDescriptorSet;
            descriptorWrite.dstBinding       = binding;
            descriptorWrite.dstArrayElement  = 0;  // This offsets into BufferInfos/ImageInfos, which always starts at 0 in this class.
            descriptorWrite.descriptorType   = descriptorInfo.DescriptorType;
            descriptorWrite.descriptorCount  = descriptorInfo.DescriptorCount;
            descriptorWrite.pBufferInfo      = nullptr;
            descriptorWrite.pImageInfo       = nullptr;
            descriptorWrite.pTexelBufferView = nullptr;  // TODO: whats a TexelBufferView?


            if(descriptorInfo.BufferInfos.size() > 0)
            {
                descriptorWrite.pBufferInfo = descriptorInfo.BufferInfos.data();
            }
            else if(descriptorInfo.ImageInfos.size() > 0)
            {
                descriptorWrite.pImageInfo = descriptorInfo.ImageInfos.data();
            }
            else
            {
                descriptorWrite.pNext = pairBindingDescriptorInfo.second.pNext;
            }

            descriptorWrites.push_back(descriptorWrite);
        }
        vkUpdateDescriptorSets(mContext->Device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void DescriptorSet::Destroy()
    {
        if(mDescriptorPool != VK_NULL_HANDLE)
        {
            mMapBindingToDescriptorInfo.clear();
            vkDestroyDescriptorPool(mContext->Device(), mDescriptorPool, nullptr);
            mDescriptorPool = VK_NULL_HANDLE;
            mDescriptorSet  = VK_NULL_HANDLE;
        }
        if(mDescriptorSetLayout != VK_NULL_HANDLE && !mExternalLayout)
        {
            vkDestroyDescriptorSetLayout(mContext->Device(), mDescriptorSetLayout, nullptr);
            mDescriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    void DescriptorSet::SetDescriptorAt(uint32_t binding, const std::vector<const ManagedBuffer*>& buffers, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        AssertBindingInUse(binding);
        uint32_t                            count = buffers.size();
        std::vector<VkDescriptorBufferInfo> bufferInfos(count);
        for(size_t i = 0; i < count; i++)
        {
            buffers[i]->FillVkDescriptorBufferInfo(bufferInfos[i]);
            AssertHandleNotNull(bufferInfos[i].buffer, binding);
        }

        mMapBindingToDescriptorInfo[binding] = {.BufferInfos = bufferInfos, .DescriptorType = descriptorType, .DescriptorCount = count, .ShaderStageFlags = shaderStageFlags};
    }

    void DescriptorSet::SetDescriptorAt(uint32_t binding, const ManagedBuffer& buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        AssertBindingInUse(binding);
        AssertHandleNotNull(buffer.GetBuffer(), binding);
        mMapBindingToDescriptorInfo[binding] = {
            .BufferInfos = {buffer.GetVkDescriptorBufferInfo()}, .DescriptorType = descriptorType, .DescriptorCount = 1U, .ShaderStageFlags = shaderStageFlags};
    }

    void DescriptorSet::SetDescriptorAt(uint32_t binding, const ManagedBuffer* buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        SetDescriptorAt(binding, *buffer, descriptorType, shaderStageFlags);
    }

    void DescriptorSet::SetDescriptorAt(uint32_t                                binding,
                                        const std::vector<const ManagedImage*>& images,
                                        VkImageLayout                           layout,
                                        VkSampler                               sampler,
                                        VkDescriptorType                        descriptorType,
                                        VkShaderStageFlags                      shaderStageFlags)
    {

        AssertBindingInUse(binding);

        uint32_t                           count = images.size();
        std::vector<VkDescriptorImageInfo> imageInfos(count);
        for(size_t i = 0; i < count; i++)
        {
            imageInfos[i].imageView   = images[i]->GetImageView();
            imageInfos[i].imageLayout = layout;
            imageInfos[i].sampler     = sampler;
            AssertHandleNotNull(imageInfos[i].imageView, binding);
        }
        mMapBindingToDescriptorInfo[binding] = {.ImageInfos = imageInfos, .DescriptorType = descriptorType, .DescriptorCount = count, .ShaderStageFlags = shaderStageFlags};
    }
    void DescriptorSet::SetDescriptorAt(
        uint32_t binding, const ManagedImage* image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        SetDescriptorAt(binding, *image, layout, sampler, descriptorType, shaderStageFlags);
    }
    void DescriptorSet::SetDescriptorAt(
        uint32_t binding, const ManagedImage& image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        AssertBindingInUse(binding);
        AssertHandleNotNull(image.GetImageView(), binding);
        mMapBindingToDescriptorInfo[binding] = {.ImageInfos       = {VkDescriptorImageInfo{.sampler = sampler, .imageView = image.GetImageView(), .imageLayout = layout}},
                                                .DescriptorType   = descriptorType,
                                                .DescriptorCount  = 1U,
                                                .ShaderStageFlags = shaderStageFlags};
    }

    void DescriptorSet::SetDescriptorAt(uint32_t                                  binding,
                                        const std::vector<VkDescriptorImageInfo>& imageInfos,
                                        VkDescriptorType                          descriptorType,
                                        VkShaderStageFlags                        shaderStageFlags)
    {
        AssertBindingInUse(binding);
        for(const VkDescriptorImageInfo& imageInfo : imageInfos)
        {
            AssertHandleNotNull(imageInfo.imageView, binding);
        }
        mMapBindingToDescriptorInfo[binding] = {
            .ImageInfos = imageInfos, .DescriptorType = descriptorType, .DescriptorCount = static_cast<uint32_t>(imageInfos.size()), .ShaderStageFlags = shaderStageFlags};
    }
    void DescriptorSet::SetDescriptorAt(uint32_t                                   binding,
                                        const std::vector<VkDescriptorBufferInfo>& bufferInfos,
                                        VkDescriptorType                           descriptorType,
                                        VkShaderStageFlags                         shaderStageFlags)
    {
        AssertBindingInUse(binding);
        for(const VkDescriptorBufferInfo& bufferInfo : bufferInfos)
        {
            AssertHandleNotNull(bufferInfo.buffer, binding);
        }
        mMapBindingToDescriptorInfo[binding] = {
            .BufferInfos = bufferInfos, .DescriptorType = descriptorType, .DescriptorCount = static_cast<uint32_t>(bufferInfos.size()), .ShaderStageFlags = shaderStageFlags};
    }
    void DescriptorSet::SetDescriptorAt(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        AssertBindingInUse(binding);
        AssertHandleNotNull(imageInfo.imageView, binding);
        mMapBindingToDescriptorInfo[binding] = {.ImageInfos = {imageInfo}, .DescriptorType = descriptorType, .DescriptorCount = 1U, .ShaderStageFlags = shaderStageFlags};
    }
    void DescriptorSet::SetDescriptorAt(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        AssertBindingInUse(binding);
        AssertHandleNotNull(bufferInfo.buffer, binding);
        mMapBindingToDescriptorInfo[binding] = {.BufferInfos = {bufferInfo}, .DescriptorType = descriptorType, .DescriptorCount = 1U, .ShaderStageFlags = shaderStageFlags};
    }

    void DescriptorSet::SetDescriptorAt(uint32_t binding, void* pNext, uint32_t DescriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        AssertBindingInUse(binding);
        AssertHandleNotNull(pNext, binding);
        mMapBindingToDescriptorInfo[binding] = {.pNext = pNext, .DescriptorType = descriptorType, .DescriptorCount = DescriptorCount, .ShaderStageFlags = shaderStageFlags};
    }

    void DescriptorSet::CreateDescriptorSet()
    {
        uint32_t numSets = 1;
        // --------------------------------------------------------------------------------------------
        // define which descriptors need to be allocated from a descriptor pool, based on the created
        // flag in the shared descriptor info.
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(mMapBindingToDescriptorInfo.size());

        for(const auto& descriptorLocation : mMapBindingToDescriptorInfo)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type            = descriptorLocation.second.DescriptorType;
            poolSize.descriptorCount = descriptorLocation.second.DescriptorCount;

            // prepare pool size
            if(poolSize.descriptorCount > 0)
            {
                poolSizes.push_back(poolSize);
            }
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = numSets;

        AssertVkResult(vkCreateDescriptorPool(mContext->Device(), &poolInfo, nullptr, &mDescriptorPool));

        // --------------------------------------------------------------------------------------------
        // allocate descriptor sets by their layout

        std::vector<VkDescriptorSetLayout> layouts(numSets, mDescriptorSetLayout);
        VkDescriptorSetAllocateInfo        descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool     = mDescriptorPool;
        descriptorSetAllocInfo.descriptorSetCount = numSets;
        descriptorSetAllocInfo.pSetLayouts        = layouts.data();

        AssertVkResult(vkAllocateDescriptorSets(mContext->Device(), &descriptorSetAllocInfo, &mDescriptorSet));

        if (mName.size() > 0)
        {
            SetVulkanObjectName(mContext, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET, mDescriptorSet, mName);
        }

        // --------------------------------------------------------------------------------------------
        // connect the descriptor sets with the descriptors
        Update();
    }

    void DescriptorSet::CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags)
    {
        // --------------------------------------------------------------------------------------------
        // create the descriptor set layout based on the given bindings
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(mMapBindingToDescriptorInfo.size());

        for(const auto& pairBindingDescriptorInfo : mMapBindingToDescriptorInfo)
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding            = pairBindingDescriptorInfo.first;
            layoutBinding.descriptorCount    = pairBindingDescriptorInfo.second.DescriptorCount;
            layoutBinding.descriptorType     = pairBindingDescriptorInfo.second.DescriptorType;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags         = pairBindingDescriptorInfo.second.ShaderStageFlags;

            layoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = layoutBindings.size();
        layoutInfo.pBindings    = layoutBindings.data();
        layoutInfo.flags        = descriptorSetLayoutCreateFlags;

        AssertVkResult(vkCreateDescriptorSetLayout(mContext->Device(), &layoutInfo, nullptr, &mDescriptorSetLayout));
    }

    void DescriptorSet::AssertBindingInUse(uint32_t binding)
    {
#ifdef FORAY_DEBUG
        // only check binding in use if descriptor set not yet created
        if(Exists())
            return;

        if(mMapBindingToDescriptorInfo.find(binding) != mMapBindingToDescriptorInfo.end())
            throw Exception("Attempted to use descriptor slot binding {} which is already in use!", binding);
#endif
    }

    void DescriptorSet::AssertHandleNotNull(void* handle, uint32_t binding)
    {
#ifdef FORAY_DEBUG
        if(handle == VK_NULL_HANDLE)
        {
            throw Exception("Attempted to bind VK_NULL_HANDLE on descriptor with binding index {}", binding);
        }
#endif
    }
}  // namespace foray::core