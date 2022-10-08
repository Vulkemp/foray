#include "foray_materialbuffer.hpp"

namespace foray::scene {
    MaterialBuffer::MaterialBuffer(const core::VkContext* context) : mBuffer(context, false)
    {
        mBuffer.GetBuffer().SetName("MaterialBuffer");
        mDescriptorBufferInfos.resize(1);
    }
    void MaterialBuffer::UpdateDeviceLocal()
    {
        if(!mBuffer.GetVector().size())
        {
            // Push something empty so we don't fail to write to the descriptor set down the line
            mBuffer.GetVector().push_back(DefaultMaterialEntry{});
        }
        mBuffer.InitOrUpdate();
    }
    void MaterialBuffer::Destroy()
    {
        mBuffer.Destroy();
    }

    std::shared_ptr<core::DescriptorSetHelper::DescriptorInfo> MaterialBuffer::GetDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        auto descriptorInfo = std::make_shared<core::DescriptorSetHelper::DescriptorInfo>();
        mBuffer.GetBuffer().FillVkDescriptorBufferInfo(&mDescriptorBufferInfos[0]);
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage, &mDescriptorBufferInfos);
        return descriptorInfo;
    }

}  // namespace foray