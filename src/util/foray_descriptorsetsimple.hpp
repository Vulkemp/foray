#pragma once
#include "../core/foray_descriptorset.hpp"
#include "../foray_mem.hpp"

namespace foray::util {
    class DescriptorSetSimple
    {
      public:
        explicit DescriptorSetSimple(core::Context* context = nullptr);
        virtual ~DescriptorSetSimple();

        /// @brief Set a binding (image info array)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfos, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer info array)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const std::vector<VkDescriptorBufferInfo>& bufferInfos, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);


        /// @brief Set a binding (buffer array)
        /// @param binding binding index
        /// @param buffers vector of buffers. Is converted to a VkDescriptorBufferInfos vector.
        void SetDescriptorAt(uint32_t binding, const std::vector<const core::ManagedBuffer*>& buffers, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer)
        /// @param binding binding index
        /// @param buffer buffer. Is converted to a VkDescriptorBufferInfo object.
        void SetDescriptorAt(uint32_t binding, const core::ManagedBuffer* buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (storage image array)
        /// @param binding binding index
        /// @param images image array. Is converted to a VkDescriptorImageInfo vector (shared layout and sampler).
        /// @param layout image layout as accessed from shader
        /// @param sampler shared sampler
        void SetDescriptorAt(uint32_t binding, const std::vector<const core::ManagedImage*>& images, VkImageLayout layout, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (image)
        /// @param binding binding index
        /// @param images image. Is converted to a VkDescriptorImageInfo object (with layout and sampler).
        /// @param layout image layout as accessed from shader
        void SetDescriptorAt(
            uint32_t binding, const core::ManagedImage* image, VkImageLayout layout, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (image)
        /// @param binding binding index
        /// @param images image. Is converted to a VkDescriptorImageInfo object (with layout and sampler).
        /// @param layout image layout as accessed from shader
        void SetDescriptorAt(
            uint32_t binding, const core::ManagedImage* image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (image info)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer info)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (CombinedImageSampler)
        /// @param binding binding index
        /// @param sampledImage Image + Sampler combo
        /// @param layout ImageLayout
        void SetDescriptorAt(uint32_t binding, const core::CombinedImageSampler* sampledImage, VkImageLayout layout, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (CombinedImageSampler)
        /// @param binding binding index
        /// @param sampledImage Image + Sampler vector (shared layout)
        /// @param layout ImageLayout
        void SetDescriptorAt(uint32_t binding, const std::vector<const core::CombinedImageSampler*>& sampledImages, VkImageLayout layout, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (AccelerationStructure)
        /// @param binding binding index
        /// @param accelerationStructure Acceleration Structure
        /// @param layout ImageLayout
        void SetDescriptorAt(uint32_t binding, VkAccelerationStructureKHR accelerationStructure, VkShaderStageFlags shaderStageFlags);

        void CreateOrUpdate(core::Context* context, std::string debugName);
        void Create(core::Context* context, std::string debugName);
        void Update();
        void Destroy();

        bool Exists() const { return mSet; }

        FORAY_PROPERTY_V(Context)

        VkDescriptorSetLayout GetLayout() const { return mLayout->GetLayout(); }
        VkDescriptorSet       GetSet() const { return mSet->GetSet(); }

      protected:
        core::Context*                mContext = nullptr;
        std::string                   mName;
        Local<core::DescriptorLayout> mLayout;
        Local<core::DescriptorPool>   mPool;
        Local<core::DescriptorSet>    mSet;

        void PrepareBinding(uint32_t binding, uint32_t count, VkDescriptorType type);

      private:
        std::vector<core::DescriptorBindingBase*> mBindings;
    };
}  // namespace foray::util
