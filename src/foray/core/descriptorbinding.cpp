#include "descriptorbinding.hpp"
#include "../as/tlas.hpp"
#include "../exception.hpp"
#include "managedbuffer.hpp"
#include "imageview.hpp"

namespace foray::core {

#define VALIDATE(test, message)                                                                                                                                                    \
    if(!(test))                                                                                                                                                                    \
    {                                                                                                                                                                              \
        out_message = message;                                                                                                                                                     \
        return false;                                                                                                                                                              \
    }

    bool DescriptorBindingBase::ValidateForBind(std::string& out_message) const
    {
        VALIDATE(mCount > 0, "Count must be > 0")
        VALIDATE(mStageFlags > 0, "Stageflags not set")
        return true;
    }

    VkDescriptorSetLayoutBinding DescriptorBindingBase::GetBinding() const
    {
        // clang-format off
        return VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = mType,
            .descriptorCount = mCount,
            .stageFlags = mStageFlags,
            .pImmutableSamplers = nullptr,
        };
        // clang-format on
    }

    DescriptorBindingImageBase::DescriptorBindingImageBase(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t count, vk::Sampler* immutableSamplers)
        : DescriptorBindingBase(type, stageFlags, count), mImageInfos(count)
    {
        if(!!immutableSamplers)
        {
            mImmutableSamplers.resize(count);
            memcpy(mImmutableSamplers.data(), immutableSamplers, count);
        }
    }

    bool DescriptorBindingImageBase::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingBase::ValidateForBind(out_message))
        {
            return false;
        }
        VALIDATE(mImageInfos.size() == mCount, "Image Info array size must match count")
        VALIDATE((mImmutableSamplers.size() == mCount || mImmutableSamplers.size() == 0), "Immutable samplers array size must be zero or equal count")
        return true;
    }

    VkDescriptorSetLayoutBinding DescriptorBindingImageBase::GetBinding() const
    {
        // clang-format off
        return VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = mType,
            .descriptorCount = mCount,
            .stageFlags = mStageFlags,
            .pImmutableSamplers = mImmutableSamplers.data()
        };
        // clang-format on
    }

    VkWriteDescriptorSet DescriptorBindingImageBase::GetState() const
    {
        // clang-format off
        return VkWriteDescriptorSet{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .descriptorCount = mCount,
            .descriptorType = mType,
            .pImageInfo = mImageInfos.data()
        };
        // clang-format on
    }

    DescriptorBindingImageBase& DescriptorBindingImageBase::SetState(uint32_t count, const vk::DescriptorImageInfo* imageInfos)
    {
        Assert(mImmutableSamplers.size() == 0, "Cannot set state of a Sampler descriptor if immutable samplers are set");
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!imageInfos, "unexpected nullptr");
        switch(mType)
        {
            case VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER: {
                for(uint32_t i = 0; i < count; i++)
                {
                    Assert(!!imageInfos[i].sampler, "Unexpected nullptr");
                }
                break;
            }
            case VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                for(uint32_t i = 0; i < count; i++)
                {
                    Assert(!!imageInfos[i].sampler, "Unexpected nullptr");
                    Assert(!!imageInfos[i].imageView, "Unexpected nullptr");
                }
                break;
            }
            case VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                for(uint32_t i = 0; i < count; i++)
                {
                    Assert(!!imageInfos[i].imageView, "Unexpected nullptr");
                }
                break;
            }
            case VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                for(uint32_t i = 0; i < count; i++)
                {
                    Assert(!!imageInfos[i].imageView, "Unexpected nullptr");
                }
                break;
            }
            default:
                FORAY_THROWFMT("Unable to handle descriptorType {}", NAMEOF_ENUM(mType))
        }
        memcpy(mImageInfos.data(), imageInfos, count * sizeof(vk::DescriptorImageInfo));
        return *this;
    }


    DescriptorBindingSampler::DescriptorBindingSampler(VkShaderStageFlags stageFlags, uint32_t count, vk::Sampler* immutableSamplers)
        : DescriptorBindingImageBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER, stageFlags, count, immutableSamplers)
    {
    }

    bool DescriptorBindingSampler::ShouldWrite() const
    {
        return mImmutableSamplers.size() == 0;
    }


    bool DescriptorBindingSampler::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingImageBase::ValidateForWrite(out_message))
        {
            return false;
        }
        if(mImmutableSamplers.size() == 0)
        {
            for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
            {
                const vk::DescriptorImageInfo& info = mImageInfos[i];
                VALIDATE(!!info.sampler, fmt::format("Index {}: Sampler may not be nullptr!", i))
            }
        }
        else
        {
            for(uint32_t i = 0; i < (uint32_t)mImmutableSamplers.size(); i++)
            {
                VALIDATE(!!mImmutableSamplers[i], fmt::format("Index {}: Immutable sampler may not be nullptr!", i))
            }
        }
        for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
        {
            const vk::DescriptorImageInfo& info = mImageInfos[i];
            VALIDATE(!info.imageView, fmt::format("Index {}: ImageView must be nullptr!", i))
        }
        return true;
    }

    DescriptorBindingSampler& DescriptorBindingSampler::SetState(uint32_t count, vk::Sampler* samplers)
    {
        Assert(mImmutableSamplers.size() == 0, "Cannot set state of a Sampler descriptor if immutable samplers are set");
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!samplers, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!samplers[i], "Index {}: Sampler is nullptr", i)
            mImageInfos[i] = vk::DescriptorImageInfo{.sampler = samplers[i]};
        }
        return *this;
    }

    DescriptorBindingSampler& DescriptorBindingSampler::SetState(uint32_t count, const SamplerReference* samplers)
    {
        Assert(!!samplers, "samplers is nullptr!");
        std::vector<vk::Sampler> samplers2(count);
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!samplers[i].GetSampler(), "Index {}: Sampler is nullptr", i)
            samplers2[i] = samplers[i].GetSampler();
        }
        return SetState(count, samplers2.data());
    }

    DescriptorBindingSampler& DescriptorBindingSampler::SetState(vk::Sampler sampler)
    {
        return SetState(1u, &sampler);
    }

    DescriptorBindingSampler& DescriptorBindingSampler::SetState(const SamplerReference* sampler)
    {
        return SetState(sampler->GetSampler());
    }

    DescriptorBindingCombinedImageSampler::DescriptorBindingCombinedImageSampler(VkShaderStageFlags stageFlags, uint32_t count, vk::Sampler* immutableSamplers)
        : DescriptorBindingImageBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, count, immutableSamplers)
    {
    }

    bool DescriptorBindingCombinedImageSampler::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingImageBase::ValidateForWrite(out_message))
        {
            return false;
        }
        if(mImmutableSamplers.size() == 0)
        {
            for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
            {
                const vk::DescriptorImageInfo& info = mImageInfos[i];
                VALIDATE(!!info.sampler, fmt::format("Index {}: Sampler may not be nullptr!", i))
            }
        }
        else
        {
            for(uint32_t i = 0; i < (uint32_t)mImmutableSamplers.size(); i++)
            {
                VALIDATE(!!mImmutableSamplers[i], fmt::format("Index {}: Immutable sampler may not be nullptr!", i))
            }
        }
        for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
        {
            const vk::DescriptorImageInfo& info = mImageInfos[i];
            VALIDATE(!!info.imageView, fmt::format("Index {}: ImageView must not be nullptr!", i))
        }
        return true;
    }

    DescriptorBindingCombinedImageSampler& DescriptorBindingCombinedImageSampler::SetState(uint32_t count, const CombinedImageSampler* combinedImageSamplers, vk::ImageLayout layout)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!combinedImageSamplers, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            const CombinedImageSampler& cis = combinedImageSamplers[i];
            FORAY_ASSERTFMT(!!cis.GetSampler(), "Index {}: Sampler is nullptr", i)
            FORAY_ASSERTFMT(!!cis.GetImageView(), "Index {}: Image is nullptr", i)
            mImageInfos[i] = vk::DescriptorImageInfo{.sampler = cis.GetSampler(), .imageView = cis.GetImageView()->GetView(), .imageLayout = layout};
        }
        return *this;
    }

    DescriptorBindingCombinedImageSampler& DescriptorBindingCombinedImageSampler::SetState(const CombinedImageSampler* combinedImageSampler, vk::ImageLayout layout)
    {
        return SetState(1u, combinedImageSampler, layout);
    }

    DescriptorBindingStorageImage::DescriptorBindingStorageImage(VkShaderStageFlags stageFlags, uint32_t count)
        : DescriptorBindingImageBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, stageFlags, count)
    {
    }

    bool DescriptorBindingStorageImage::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingImageBase::ValidateForWrite(out_message))
        {
            return false;
        }
        VALIDATE(mImmutableSamplers.size() == 0, "Storage Image descriptor bindings may not have samplers")
        for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
        {
            const vk::DescriptorImageInfo& info = mImageInfos[i];
            VALIDATE(!!info.imageView, fmt::format("Index {}: ImageView must not be nullptr!", i))
            VALIDATE(!info.sampler, fmt::format("Index {}: Sampler must be nullptr!", i))
        }
        return true;
    }

    DescriptorBindingStorageImage& DescriptorBindingStorageImage::SetState(uint32_t count, vk::ImageView* views, vk::ImageLayout layout)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!views, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!views[i], "Index {}: ImageView is nullptr!", i)
            mImageInfos[i] = vk::DescriptorImageInfo{.imageView = views[i], .imageLayout = layout};
        }
        return *this;
    }

    DescriptorBindingStorageImage& DescriptorBindingStorageImage::SetState(vk::ImageView view, vk::ImageLayout layout)
    {
        return SetState(1u, &view, layout);
    }

    DescriptorBindingStorageImage& DescriptorBindingStorageImage::SetState(const ImageViewRef* image, vk::ImageLayout layout)
    {
        vk::ImageView view = image->GetView();
        return SetState(1u, &view, layout);
    }

    DescriptorBindingSampledImage::DescriptorBindingSampledImage(VkShaderStageFlags stageFlags, uint32_t count)
        : DescriptorBindingImageBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, stageFlags, count)
    {
    }

    bool DescriptorBindingSampledImage::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingImageBase::ValidateForWrite(out_message))
        {
            return false;
        }
        VALIDATE(mImmutableSamplers.size() == 0, "Storage Image descriptor bindings may not have samplers")
        for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
        {
            const vk::DescriptorImageInfo& info = mImageInfos[i];
            VALIDATE(!!info.imageView, fmt::format("Index {}: ImageView must not be nullptr!", i))
            VALIDATE(!info.sampler, fmt::format("Index {}: Sampler must be nullptr!", i))
        }
        return true;
    }

    DescriptorBindingSampledImage& DescriptorBindingSampledImage::SetState(uint32_t count, vk::ImageView* views, vk::ImageLayout layout)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!views, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!views[i], "Index {}: ImageView is nullptr!", i)
            mImageInfos[i] = vk::DescriptorImageInfo{.imageView = views[i], .imageLayout = layout};
        }
        return *this;
    }

    DescriptorBindingSampledImage& DescriptorBindingSampledImage::SetState(vk::ImageView view, vk::ImageLayout layout)
    {
        return SetState(1u, &view, layout);
    }

    DescriptorBindingSampledImage& DescriptorBindingSampledImage::SetState(const ImageViewRef* image, vk::ImageLayout layout)
    {
        vk::ImageView view = image->GetView();
        return SetState(1u, &view, layout);
    }

    DescriptorBindingInputAttachment::DescriptorBindingInputAttachment(uint32_t count)
            : DescriptorBindingImageBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, vk::ShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, count)
    {
    }

    bool DescriptorBindingInputAttachment::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingImageBase::ValidateForWrite(out_message))
        {
            return false;
        }
        VALIDATE(mImmutableSamplers.size() == 0, "Storage Image descriptor bindings may not have samplers")
        for(uint32_t i = 0; i < (uint32_t)mImageInfos.size(); i++)
        {
            const vk::DescriptorImageInfo& info = mImageInfos[i];
            VALIDATE(!!info.imageView, fmt::format("Index {}: ImageView must not be nullptr!", i))
            VALIDATE(!info.sampler, fmt::format("Index {}: Sampler must be nullptr!", i))
        }
        return true;
    }

    DescriptorBindingInputAttachment& DescriptorBindingInputAttachment::SetState(uint32_t count, vk::ImageView* views, vk::ImageLayout layout)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!views, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!views[i], "Index {}: ImageView is nullptr!", i)
            mImageInfos[i] = vk::DescriptorImageInfo{.imageView = views[i], .imageLayout = layout};
        }
        return *this;
    }

    DescriptorBindingInputAttachment& DescriptorBindingInputAttachment::SetState(vk::ImageView view, vk::ImageLayout layout)
    {
        return SetState(1u, &view, layout);
    }

    DescriptorBindingInputAttachment& DescriptorBindingInputAttachment::SetState(const ImageViewRef* image, vk::ImageLayout layout)
    {
        vk::ImageView view = image->GetView();
        return SetState(1u, &view, layout);
    }

    DescriptorBindingBufferBase::DescriptorBindingBufferBase(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t count)
        : DescriptorBindingBase(type, stageFlags, count), mBufferInfos(count)
    {
    }

    bool DescriptorBindingBufferBase::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingBase::ValidateForBind(out_message))
        {
            return false;
        }
        VALIDATE(mBufferInfos.size() == mCount, "Image Info array size must match count")
        for(uint32_t i = 0; i < mCount; i++)
        {
            const vk::DescriptorBufferInfo& info = mBufferInfos[i];
            VALIDATE(!!info.buffer, fmt::format("Index {}: Buffer must not be nullptr!", i))
            VALIDATE(info.range > 0 || info.range == VK_WHOLE_SIZE, fmt::format("Index {}: Size must not be zero!", i))
        }
        return true;
    }

    VkWriteDescriptorSet DescriptorBindingBufferBase::GetState() const
    {
        // clang-format off
        return VkWriteDescriptorSet{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .descriptorCount = mCount,
            .descriptorType = mType,
            .pBufferInfo = mBufferInfos.data()
        };
        // clang-format on
    }

    DescriptorBindingBufferBase& DescriptorBindingBufferBase::SetState(uint32_t count, vk::Buffer* buffers)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!buffers, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!buffers[i], "Index {}: Buffer is nullptr!", i)
            mBufferInfos[i] = vk::DescriptorBufferInfo{.buffer = buffers[i], .range = VK_WHOLE_SIZE};
        }
        return *this;
    }

    DescriptorBindingBufferBase& DescriptorBindingBufferBase::SetState(vk::Buffer buffer)
    {
        return SetState(1u, &buffer);
    }

    DescriptorBindingBufferBase& DescriptorBindingBufferBase::SetState(uint32_t count, const vk::DescriptorBufferInfo* bufferInfos)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!bufferInfos, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!bufferInfos[i].buffer, "Index {}: Buffer is nullptr!", i)
            mBufferInfos[i] = bufferInfos[i];
        }
        return *this;
    }

    DescriptorBindingBufferBase& DescriptorBindingBufferBase::SetState(const ManagedBuffer* buffer)
    {
        return SetState(buffer->GetBuffer());
    }

    DescriptorBindingStorageBuffer::DescriptorBindingStorageBuffer(VkShaderStageFlags stageFlags, uint32_t count)
        : DescriptorBindingBufferBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlags, count)
    {
    }

    DescriptorBindingStorageBuffer::DescriptorBindingStorageBuffer(VkShaderStageFlags stageFlags, vk::Buffer buffer)
     : DescriptorBindingBufferBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlags, 1u)
    {
        SetState(buffer);
    }

    DescriptorBindingUniformBuffer::DescriptorBindingUniformBuffer(VkShaderStageFlags stageFlags, uint32_t count)
        : DescriptorBindingBufferBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags, count)
    {
    }

    DescriptorBindingUniformBuffer::DescriptorBindingUniformBuffer(VkShaderStageFlags stageFlags, vk::Buffer buffer) 
     : DescriptorBindingBufferBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags, 1u)
    {
        SetState(buffer);
    }

    DescriptorBindingAccelerationStructure::DescriptorBindingAccelerationStructure(VkShaderStageFlags stageFlags, uint32_t count)
        : DescriptorBindingBase(VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, stageFlags, count)
        , mAccelerationStructures(count)
        , mDescriptorPNext(VkWriteDescriptorSetAccelerationStructureKHR{.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                                                                        .accelerationStructureCount = count,
                                                                        .pAccelerationStructures    = mAccelerationStructures.data()})
    {
    }

    DescriptorBindingAccelerationStructure::DescriptorBindingAccelerationStructure(VkShaderStageFlags stageFlags, vk::AccelerationStructureKHR accelerationStructure)
        : DescriptorBindingAccelerationStructure(stageFlags, 1u)
    {
        SetState(accelerationStructure);
    }

    bool DescriptorBindingAccelerationStructure::ValidateForWrite(std::string& out_message) const
    {
        if(!DescriptorBindingBase::ValidateForBind(out_message))
        {
            return false;
        }
        VALIDATE(mDescriptorPNext.pAccelerationStructures == mAccelerationStructures.data(), "PNext acceleration structure pointer not kept up to date");
        VALIDATE(mDescriptorPNext.accelerationStructureCount == (uint32_t)mAccelerationStructures.size(), "PNext acceleration structure count incorrect");
        VALIDATE(mAccelerationStructures.size() == mCount, "Image Info array size must match count")
        for(uint32_t i = 0; i < mCount; i++)
        {
            VALIDATE(!!mAccelerationStructures[i], fmt::format("Index {}: Acceleration Structure must not be nullptr!", i))
        }
        return true;
    }

    VkWriteDescriptorSet DescriptorBindingAccelerationStructure::GetState() const
    {
        // clang-format off
        return VkWriteDescriptorSet{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = &mDescriptorPNext,
            .descriptorCount = mCount,
            .descriptorType = mType,
        };
        // clang-format on
    }

    DescriptorBindingAccelerationStructure& DescriptorBindingAccelerationStructure::SetState(uint32_t count, vk::AccelerationStructureKHR* accelerationStructures)
    {
        Assert(mCount == count, "Input count vs. descriptor count mismatch!");
        Assert(!!accelerationStructures, "unexpected nullptr");
        for(uint32_t i = 0; i < count; i++)
        {
            FORAY_ASSERTFMT(!!accelerationStructures[i], "Index {}: Acceleration Structure is nullptr!", i)
            mAccelerationStructures[i] = accelerationStructures[i];
        }
        return *this;
    }

    DescriptorBindingAccelerationStructure& DescriptorBindingAccelerationStructure::SetState(vk::AccelerationStructureKHR accelerationStructure)
    {
        return SetState(1u, &accelerationStructure);
    }

    DescriptorBindingAccelerationStructure& DescriptorBindingAccelerationStructure::SetState(const as::Tlas* accelerationStructure)
    {
        return SetState(accelerationStructure->GetAccelerationStructure());
    }

}  // namespace foray::core
