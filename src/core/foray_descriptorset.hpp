#pragma once
#include "foray_managedbuffer.hpp"
#include "foray_managedimage.hpp"
#include "foray_managedresource.hpp"
#include <unordered_map>

namespace foray::core {

    /// @brief Helps with the creation of a VkDescriptorSetLayout and VkDescriptorSet.
    /// Not supported
    /// - immutable samplers
    /// - descriptorSetLayout pNext
    class DescriptorSet : public VulkanResource<VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET>
    {
      public:
        /// @brief Creates the VkDescriptorSet
        /// @param
        /// @param createLayout
        void         Create(Context*                         context,
                            std::string                      debugName,
                            VkDescriptorSetLayout            predefinedLayout               = VK_NULL_HANDLE,
                            VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags = 0);
        void         Update();
        virtual void Destroy() override;
        ~DescriptorSet() { Destroy(); }

        void SetDescriptorAt(uint32_t binding, const std::vector<const ManagedBuffer*>& buffers, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        void SetDescriptorAt(uint32_t binding, const ManagedBuffer& buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        void SetDescriptorAt(uint32_t binding, const ManagedBuffer* buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        void SetDescriptorAt(uint32_t                                binding,
                             const std::vector<const ManagedImage*>& images,
                             VkImageLayout                           layout,
                             VkSampler                               sampler,
                             VkDescriptorType                        descriptorType,
                             VkShaderStageFlags                      shaderStageFlags);
        void SetDescriptorAt(
            uint32_t binding, const ManagedImage* image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        void SetDescriptorAt(
            uint32_t binding, const ManagedImage& image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        void SetDescriptorAt(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfos, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        void SetDescriptorAt(uint32_t binding, const std::vector<VkDescriptorBufferInfo>& bufferInfos, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        void SetDescriptorAt(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        void SetDescriptorAt(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        void SetDescriptorAt(uint32_t binding, void* pNext, uint32_t DescriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);


        bool Exists() const override { return mDescriptorSet != VK_NULL_HANDLE; }

        FORAY_PROPERTY_GET(DescriptorSet)
        FORAY_PROPERTY_GET(DescriptorSetLayout)

      protected:
        struct DescriptorInfo
        {
            std::vector<VkDescriptorBufferInfo> BufferInfos;
            std::vector<VkDescriptorImageInfo>  ImageInfos;
            void*                               pNext{nullptr};
            VkDescriptorType                    DescriptorType;
            uint32_t                            DescriptorCount;
            VkShaderStageFlags                  ShaderStageFlags;
        };

        std::unordered_map<uint32_t, DescriptorInfo> mMapBindingToDescriptorInfo;
        Context*                                     mContext{};
        VkDescriptorPool                             mDescriptorPool{};
        VkDescriptorSetLayout                        mDescriptorSetLayout{};
        VkDescriptorSet                              mDescriptorSet{};

        void CreateDescriptorSet();
        void CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags);
    };
}  // namespace foray::core