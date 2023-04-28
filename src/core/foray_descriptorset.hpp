#pragma once
#include "foray_managedbuffer.hpp"
#include "foray_managedimage.hpp"
#include "foray_managedresource.hpp"
#include "foray_managed3dimage.hpp"
#include <unordered_map>

namespace foray::core {

    /// @brief Helps with the creation of a VkDescriptorSetLayout and VkDescriptorSet.
    /// @details
    /// Not supported:
    /// - immutable samplers
    /// - descriptorSetLayout pNext
    class DescriptorSet : public VulkanResource<VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET>
    {
      public:
        /// @brief Creates the VkDescriptorSet
        /// @param context Requires Device, DispatchTable
        /// @param debugName Debug object name
        /// @param predefinedLayout Allows reusing an already defined layout
        /// @param descriptorSetLayoutCreateFlags flags
        void Create(Context*                         context,
                    std::string                      debugName,
                    VkDescriptorSetLayout            predefinedLayout               = VK_NULL_HANDLE,
                    VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags = 0);
        /// @brief Rather than reallocating the descriptorset, rewrites all bindings to the descriptor set
        void Update();
        /// @brief Destroys descriptorset and layout (latter only if also allocated by this object)
        virtual void Destroy() override;
        ~DescriptorSet() { Destroy(); }

        /// @brief Set a binding (buffer array)
        /// @param binding binding index
        /// @param buffers vector of buffers. Is converted to a VkDescriptorBufferInfos vector.
        void SetDescriptorAt(uint32_t binding, const std::vector<const ManagedBuffer*>& buffers, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer)
        /// @param binding binding index
        /// @param buffer buffer. Is converted to a VkDescriptorBufferInfo object.
        void SetDescriptorAt(uint32_t binding, const ManagedBuffer& buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer)
        /// @param binding binding index
        /// @param buffer buffer. Is converted to a VkDescriptorBufferInfo object.
        void SetDescriptorAt(uint32_t binding, const ManagedBuffer* buffer, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (image array)
        /// @param binding binding index
        /// @param images image array. Is converted to a VkDescriptorImageInfo vector (shared layout and sampler).
        /// @param layout image layout as accessed from shader
        /// @param sampler shared sampler
        void SetDescriptorAt(uint32_t                                binding,
                             const std::vector<const ManagedImage*>& images,
                             VkImageLayout                           layout,
                             VkSampler                               sampler,
                             VkDescriptorType                        descriptorType,
                             VkShaderStageFlags                      shaderStageFlags);
        /// @brief Set a binding (image)
        /// @param binding binding index
        /// @param images image. Is converted to a VkDescriptorImageInfo object (with layout and sampler).
        /// @param layout image layout as accessed from shader
        void SetDescriptorAt(uint32_t binding, const ManagedImage *image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType,
                             VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (image)
        /// @param binding binding index
        /// @param images image. Is converted to a VkDescriptorImageInfo object (with layout and sampler).
        /// @param layout image layout as accessed from shader
        void SetDescriptorAt(uint32_t binding, const ManagedImage &image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType,
                             VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (image)
        /// @param binding binding index
        /// @param images image. Is converted to a VkDescriptorImageInfo object (with layout and sampler).
        /// @param layout image layout as accessed from shader
        void SetDescriptorAt(uint32_t binding, const Managed3dImage *image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType,
                             VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (image)
        /// @param binding binding index
        /// @param images image. Is converted to a VkDescriptorImageInfo object (with layout and sampler).
        /// @param layout image layout as accessed from shader
        void SetDescriptorAt(uint32_t binding, const Managed3dImage &image, VkImageLayout layout, VkSampler sampler, VkDescriptorType descriptorType,
                             VkShaderStageFlags shaderStageFlags);

        /// @brief Set a binding (image info array)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imageInfos, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer info array)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const std::vector<VkDescriptorBufferInfo>& bufferInfos, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (image info)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const VkDescriptorImageInfo& imageInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (buffer info)
        /// @param binding binding index
        void SetDescriptorAt(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (custom / .pNext)
        /// @param binding binding index
        /// @param pNext address assigned to VkDescriptorWrite::pNext
        void SetDescriptorAt(uint32_t binding, void* pNext, uint32_t DescriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (CombinedImageSampler)
        /// @param binding binding index
        /// @param sampledImage Image + Sampler combo
        /// @param layout ImageLayout
        void SetDescriptorAt(uint32_t binding, const CombinedImageSampler& sampledImage, VkImageLayout layout, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (CombinedImageSampler)
        /// @param binding binding index
        /// @param sampledImage Image + Sampler combo
        /// @param layout ImageLayout
        void SetDescriptorAt(uint32_t binding, const CombinedImageSampler* sampledImage, VkImageLayout layout, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);
        /// @brief Set a binding (CombinedImageSampler)
        /// @param binding binding index
        /// @param sampledImage Image + Sampler vector (shared layout)
        /// @param layout ImageLayout
        void SetDescriptorAt(uint32_t binding, const std::vector<const CombinedImageSampler*>& sampledImages, VkImageLayout layout, VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags);

        bool Exists() const override { return mDescriptorSet != VK_NULL_HANDLE; }

        FORAY_GETTER_V(DescriptorSet)
        FORAY_GETTER_V(DescriptorSetLayout)

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
        bool                                         mExternalLayout = false;
        VkDescriptorSet                              mDescriptorSet{};

        void CreateDescriptorSet();
        void CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags);

        void AssertBindingInUse(uint32_t binding);
        void AssertHandleNotNull(void* handle, uint32_t binding);
    };
}  // namespace foray::core