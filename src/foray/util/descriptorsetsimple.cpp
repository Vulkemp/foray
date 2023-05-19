#include "descriptorsetsimple.hpp"
#include "../core/managedbuffer.hpp"
#include "../core/managedimage.hpp"

namespace foray::util {
    DescriptorSetSimple::DescriptorSetSimple(core::Context* context) : mContext(context) {}

    DescriptorSetSimple::~DescriptorSetSimple() 
    {
        Destroy();
    }

    void DescriptorSetSimple::PrepareBinding(uint32_t binding, uint32_t count, VkDescriptorType type)
    {
        if(mSet)
        {
            core::DescriptorBindingBase* bindingObj = mBindings[binding];
            Assert(!!bindingObj);
            Assert(bindingObj->GetCount() == count);
            Assert(bindingObj->GetType() == type);
        }
        else
        {
            if(binding >= mBindings.size())
            {
                mBindings.resize(binding + 1);
            }
            Assert(!mBindings[binding], "Binding already set");
        }
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t                                  binding,
                                              const std::vector<VkDescriptorImageInfo>& imageInfos,
                                              VkDescriptorType                          descriptorType,
                                              VkShaderStageFlags                        shaderStageFlags)
    {
        uint32_t count = (uint32_t)imageInfos.size();
        PrepareBinding(binding, count, descriptorType);
        core::DescriptorBindingImageBase* bindingObj;
        if(!mSet)
        {
            switch(descriptorType)
            {
                case VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER: {
                    bindingObj         = new core::DescriptorBindingSampler(shaderStageFlags, count);
                    mBindings[binding] = bindingObj;
                    break;
                }
                case VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                    bindingObj         = new core::DescriptorBindingCombinedImageSampler(shaderStageFlags, count);
                    mBindings[binding] = bindingObj;
                    break;
                }
                case VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                    bindingObj         = new core::DescriptorBindingSampledImage(shaderStageFlags, count);
                    mBindings[binding] = bindingObj;
                    break;
                }
                case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                    bindingObj         = new core::DescriptorBindingStorageImage(shaderStageFlags, count);
                    mBindings[binding] = bindingObj;
                    break;
                }
                default:
                    FORAY_THROWFMT("Unsupported Descriptor Type {}", NAMEOF_ENUM(descriptorType));
            }
        }
        else
        {
            bindingObj = dynamic_cast<core::DescriptorBindingImageBase*>(mBindings[binding]);
        }
        bindingObj->SetState(count, imageInfos.data());
    }
    void DescriptorSetSimple::SetDescriptorAt(uint32_t                                   binding,
                                              const std::vector<VkDescriptorBufferInfo>& bufferInfos,
                                              VkDescriptorType                           descriptorType,
                                              VkShaderStageFlags                         shaderStageFlags)
    {
        uint32_t count = (uint32_t)bufferInfos.size();
        PrepareBinding(binding, count, descriptorType);
        core::DescriptorBindingBufferBase* bindingObj;
        if(mSet)
        {
            bindingObj = dynamic_cast<core::DescriptorBindingBufferBase*>(mBindings[binding]);
        }
        else
        {
            switch(descriptorType)
            {
                case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                    bindingObj         = new core::DescriptorBindingUniformBuffer(shaderStageFlags, count);
                    mBindings[binding] = bindingObj;
                    break;
                }
                case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                    bindingObj         = new core::DescriptorBindingStorageBuffer(shaderStageFlags, count);
                    mBindings[binding] = bindingObj;
                    break;
                }
                default:
                    FORAY_THROWFMT("Unsupported Descriptor Type {}", NAMEOF_ENUM(descriptorType));
            }
        }
        bindingObj->SetState(count, bufferInfos.data());
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t                                       binding,
                                              const std::vector<const core::ManagedBuffer*>& buffers,
                                              VkDescriptorType                               descriptorType,
                                              VkShaderStageFlags                             shaderStageFlags)
    {
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve(buffers.size());
        for(const core::ManagedBuffer* buffer : buffers)
        {
            bufferInfos.push_back(VkDescriptorBufferInfo{.buffer = buffer->GetBuffer(), .range = VK_WHOLE_SIZE});
        }
        SetDescriptorAt(binding, bufferInfos, descriptorType, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t binding, const core::ManagedBuffer* buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorBufferInfo> bufferInfos({VkDescriptorBufferInfo{.buffer = buffer->GetBuffer(), .range = VK_WHOLE_SIZE}});
        SetDescriptorAt(binding, bufferInfos, descriptorType, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t binding, const std::vector<const core::ManagedImage*>& images, VkImageLayout layout, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(images.size());
        for(const core::ManagedImage* image : images)
        {
            imageInfos.push_back(VkDescriptorImageInfo{.imageView = image->GetImageView(), .imageLayout = layout});
        }
        SetDescriptorAt(binding, imageInfos, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(
        uint32_t binding, const core::ManagedImage* image, VkImageLayout layout, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorImageInfo> imageInfos({VkDescriptorImageInfo{.imageView = image->GetImageView(), .imageLayout = layout}});
        SetDescriptorAt(binding, imageInfos, VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(
        uint32_t binding, const core::ManagedImage* image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorImageInfo> imageInfos({VkDescriptorImageInfo{.sampler = sampler, .imageView = image->GetImageView(), .imageLayout = layout}});
        SetDescriptorAt(binding, imageInfos, descriptorType, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorImageInfo> imageInfos({imageInfo});
        SetDescriptorAt(binding, imageInfos, descriptorType, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorBufferInfo> bufferInfos({bufferInfo});
        SetDescriptorAt(binding, bufferInfos, descriptorType, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t binding, const core::CombinedImageSampler* sampledImage, VkImageLayout layout, VkShaderStageFlags shaderStageFlags)
    {
        std::vector<VkDescriptorImageInfo> imageInfos(
            {VkDescriptorImageInfo{.sampler = sampledImage->GetSampler(), .imageView = sampledImage->GetManagedImage()->GetImageView(), .imageLayout = layout}});
        SetDescriptorAt(binding, imageInfos, VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t                                              binding,
                                              const std::vector<const core::CombinedImageSampler*>& sampledImages,
                                              VkImageLayout                                         layout,
                                              VkShaderStageFlags                                    shaderStageFlags)
    {
        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(sampledImages.size());
        for(const core::CombinedImageSampler* sampledImage : sampledImages)
        {
            imageInfos.push_back(VkDescriptorImageInfo{.sampler = sampledImage->GetSampler(), .imageView = sampledImage->GetManagedImage()->GetImageView(), .imageLayout = layout});
        }
        SetDescriptorAt(binding, imageInfos, VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStageFlags);
    }

    void DescriptorSetSimple::SetDescriptorAt(uint32_t binding, VkAccelerationStructureKHR accelerationStructure, VkShaderStageFlags shaderStageFlags)
    {
        PrepareBinding(binding, 1u, VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
        core::DescriptorBindingAccelerationStructure* bindingObj;
        if(mSet)
        {
            bindingObj = dynamic_cast<core::DescriptorBindingAccelerationStructure*>(mBindings[binding]);
        }
        else
        {
            bindingObj         = new core::DescriptorBindingAccelerationStructure(shaderStageFlags, accelerationStructure);
            mBindings[binding] = bindingObj;
        }
        bindingObj->SetState(accelerationStructure);
    }

    void DescriptorSetSimple::CreateOrUpdate(core::Context* context, std::string debugName)
    {
        if(mSet)
        {
            Update();
        }
        else
        {
            Create(context, debugName);
        }
    }

    void DescriptorSetSimple::Create(core::Context* context, std::string debugName)
    {
        Assert(!Exists());

        mContext = context;
        mName    = debugName;

        {  // Layout
            core::DescriptorLayout::Builder builder;

            for(uint32_t i = 0; i < (uint32_t)mBindings.size(); i++)
            {
                if(!!mBindings[i])
                {
                    builder.SetBinding(i, mBindings[i]);
                }
            }
            mLayout.New(mContext, builder);
        }

        {  // Pool
            core::DescriptorPool::Builder builder;
            builder.AddSets(mLayout.Get());
            mPool.New(mContext, builder);
        }

        {  // Set
            core::DescriptorSet::Builder builder;
            builder.SetLayout(mLayout.Get()).SetPool(mPool.Get());
            mSet.New(mContext, builder);
        }

        Update();
    }
    void DescriptorSetSimple::Update()
    {
        core::DescriptorSet::UpdateInfo update;

        for(core::DescriptorBindingBase* binding : mBindings)
        {
            if(!!binding)
            {
                update.AddWrite(binding);
            }
        }

        mSet->Update(update);
    }
    void DescriptorSetSimple::Destroy()
    {
        mSet.Delete();
        mPool.Delete();
        mLayout.Delete();
        for(core::DescriptorBindingBase* binding : mBindings)
        {
            if(!!binding)
            {
                delete binding;
            }
        }
        mBindings.clear();
    }
}  // namespace foray::util
