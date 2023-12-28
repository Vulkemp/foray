#pragma once
#include "../as/as_declares.hpp"
#include "../basics.hpp"
#include "../mem.hpp"
#include "../vulkan.hpp"
#include "samplercollection.hpp"

namespace foray::core {

    /// @brief Abstract base class for Descriptor Bindings, maintaining a state of the layout binding and value written to sets
    /// @remark The descriptor set maintain the final state of the bindings, setting the state multiple times on these is fine for multiple descriptor sets
    class DescriptorBindingBase
    {
      public:
        inline DescriptorBindingBase(VkDescriptorType type, VkShaderStageFlags stageFlags = 0, uint32_t count = 1) : mType(type), mStageFlags(stageFlags), mCount(count) {}
        virtual ~DescriptorBindingBase() = default;

        /// @brief Validates integrity of the stored state for binding to a layout
        /// @param out_message If returns false, contains an error message
        /// @return True if stored state is valid
        virtual bool ValidateForBind(std::string& out_message) const;
        /// @brief Validates integrity of the stored state for writing to a set
        /// @param out_message If returns false, contains an error message
        /// @return True if stored state is valid
        virtual bool ValidateForWrite(std::string& out_message) const { return true; }
        /// @brief Configures a binding struct
        /// @remark .binding index is not managed by this type
        virtual VkDescriptorSetLayoutBinding GetBinding() const;
        /// @brief Configures an update write struct
        /// @remark .dstBinding not set, .dstSet not set, .dstArrayIdx always 0
        virtual VkWriteDescriptorSet GetState() const = 0;
        /// @brief Return false, if descriptor is layout only
        virtual bool ShouldWrite() const { return true; }

        FORAY_GETTER_V(Type)
        FORAY_PROPERTY_V(Count)
        FORAY_PROPERTY_V(StageFlags)

      protected:
        const VkDescriptorType mType;
        VkShaderStageFlags     mStageFlags;
        uint32_t               mCount;
    };

    /// @brief Base class with shared code for any image/sampler descriptor binding type
    class DescriptorBindingImageBase : public DescriptorBindingBase
    {
      public:
        DescriptorBindingImageBase(VkDescriptorType type, VkShaderStageFlags stageFlags = 0, uint32_t count = 1, vk::Sampler* immutableSamplers = nullptr);

        virtual bool                         ValidateForWrite(std::string& out_message) const override;
        virtual VkDescriptorSetLayoutBinding GetBinding() const override;
        virtual VkWriteDescriptorSet         GetState() const override;
        DescriptorBindingImageBase&          SetState(uint32_t count, const vk::DescriptorImageInfo* imageInfos);

        FORAY_PROPERTY_R(ImageInfos)
        FORAY_PROPERTY_R(ImmutableSamplers)

      protected:
        std::vector<vk::DescriptorImageInfo> mImageInfos;
        std::vector<vk::Sampler>             mImmutableSamplers;
    };

    /// @brief A descriptor binding of Sampler type. Only relevant for hlsl.
    class DescriptorBindingSampler : public DescriptorBindingImageBase
    {
      public:
        DescriptorBindingSampler(VkShaderStageFlags stageFlags = 0, uint32_t count = 1, vk::Sampler* immutableSamplers = nullptr);

        virtual bool              ShouldWrite() const override;
        virtual bool              ValidateForWrite(std::string& out_message) const override;
        DescriptorBindingSampler& SetState(uint32_t count, vk::Sampler* samplers);
        DescriptorBindingSampler& SetState(uint32_t count, const SamplerReference* samplers);
        DescriptorBindingSampler& SetState(vk::Sampler sampler);
        DescriptorBindingSampler& SetState(const SamplerReference* sampler);
    };

    /// @brief A descriptor binding of a combination of sampler and image. glsl: sampler2D/sampler3D
    class DescriptorBindingCombinedImageSampler : public DescriptorBindingImageBase
    {
      public:
        DescriptorBindingCombinedImageSampler(VkShaderStageFlags stageFlags = 0, uint32_t count = 1, vk::Sampler* immutableSamplers = nullptr);

        virtual bool                           ValidateForWrite(std::string& out_message) const override;
        DescriptorBindingCombinedImageSampler& SetState(uint32_t count, const CombinedImageSampler* combinedImageSamplers, vk::ImageLayout layout);
        DescriptorBindingCombinedImageSampler& SetState(const CombinedImageSampler* combinedImageSampler, vk::ImageLayout layout);
    };

    /// @brief A descriptor binding of a storage image. glsl: image2D/image3D
    class DescriptorBindingStorageImage : public DescriptorBindingImageBase
    {
      public:
        DescriptorBindingStorageImage(VkShaderStageFlags stageFlags = 0, uint32_t count = 1);

        virtual bool                   ValidateForWrite(std::string& out_message) const override;
        DescriptorBindingStorageImage& SetState(uint32_t count, vk::ImageView* views, vk::ImageLayout layout);
        DescriptorBindingStorageImage& SetState(vk::ImageView view, vk::ImageLayout layout);
        DescriptorBindingStorageImage& SetState(const ImageViewRef* image, vk::ImageLayout layout);
    };

    /// @brief A descriptor binding of an image (not storage, but accessed by individual sampleres. only relevant in hlsl)
    class DescriptorBindingSampledImage : public DescriptorBindingImageBase
    {
      public:
        DescriptorBindingSampledImage(VkShaderStageFlags stageFlags = 0, uint32_t count = 1);

        virtual bool                   ValidateForWrite(std::string& out_message) const override;
        DescriptorBindingSampledImage& SetState(uint32_t count, vk::ImageView* views, vk::ImageLayout layout);
        DescriptorBindingSampledImage& SetState(vk::ImageView view, vk::ImageLayout layout);
        DescriptorBindingSampledImage& SetState(const ImageViewRef* image, vk::ImageLayout layout);
    };

    /// @brief A descriptor binding of an input attachment (special attachment type in rasterized passes)
    class DescriptorBindingInputAttachment : public DescriptorBindingImageBase
    {
      public:
        DescriptorBindingInputAttachment(uint32_t count = 1);

        virtual bool                   ValidateForWrite(std::string& out_message) const override;
        DescriptorBindingInputAttachment& SetState(uint32_t count, vk::ImageView* views, vk::ImageLayout layout);
        DescriptorBindingInputAttachment& SetState(vk::ImageView view, vk::ImageLayout layout);
        DescriptorBindingInputAttachment& SetState(const ImageViewRef* image, vk::ImageLayout layout);
    };

    /// @brief Base class for shared code for all supported buffer descriptor bindings
    class DescriptorBindingBufferBase : public DescriptorBindingBase
    {
      public:
        DescriptorBindingBufferBase(VkDescriptorType type, VkShaderStageFlags stageFlags = 0, uint32_t count = 1);
        DescriptorBindingBufferBase(VkDescriptorType type, VkShaderStageFlags stageFlags, vk::Buffer buffer);

        virtual bool                 ValidateForWrite(std::string& out_message) const override;
        virtual VkWriteDescriptorSet GetState() const override;

        DescriptorBindingBufferBase& SetState(uint32_t count, vk::Buffer* buffers);
        DescriptorBindingBufferBase& SetState(vk::Buffer buffer);
        DescriptorBindingBufferBase& SetState(uint32_t count, const vk::DescriptorBufferInfo* bufferInfos);
        DescriptorBindingBufferBase& SetState(const ManagedBuffer* buffer);

        FORAY_PROPERTY_R(BufferInfos)

      protected:
        std::vector<vk::DescriptorBufferInfo> mBufferInfos;
    };

    /// @brief A descriptor binding of Storage buffer type. glsl: buffer ...
    class DescriptorBindingStorageBuffer : public DescriptorBindingBufferBase
    {
      public:
        DescriptorBindingStorageBuffer(VkShaderStageFlags stageFlags = 0, uint32_t count = 1);
        DescriptorBindingStorageBuffer(VkShaderStageFlags stageFlags, vk::Buffer buffer);
    };

    /// @brief A descriptor binding of Uniform buffer type. glsl: uniform ...
    class DescriptorBindingUniformBuffer : public DescriptorBindingBufferBase
    {
      public:
        DescriptorBindingUniformBuffer(VkShaderStageFlags stageFlags = 0, uint32_t count = 1);
        DescriptorBindingUniformBuffer(VkShaderStageFlags stageFlags, vk::Buffer buffer);
    };

    /// @brief A descriptor binding of Acceleration Structure type. glsl: uniform accelerationStructureEXT
    class DescriptorBindingAccelerationStructure : public DescriptorBindingBase
    {
      public:
        DescriptorBindingAccelerationStructure(VkShaderStageFlags stageFlags = 0, uint32_t count = 1);
        DescriptorBindingAccelerationStructure(VkShaderStageFlags stageFlags, vk::AccelerationStructureKHR accelerationStructure);

        virtual bool                 ValidateForWrite(std::string& out_message) const override;
        virtual VkWriteDescriptorSet GetState() const override;

        DescriptorBindingAccelerationStructure& SetState(uint32_t count, vk::AccelerationStructureKHR* accelerationStructures);
        DescriptorBindingAccelerationStructure& SetState(vk::AccelerationStructureKHR accelerationStructure);
        DescriptorBindingAccelerationStructure& SetState(const as::Tlas* accelerationStructure);

      protected:
        std::vector<vk::AccelerationStructureKHR>      mAccelerationStructures;
        VkWriteDescriptorSetAccelerationStructureKHR mDescriptorPNext;
    };
}  // namespace foray::core