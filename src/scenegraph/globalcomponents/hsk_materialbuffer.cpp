#include "hsk_materialbuffer.hpp"

namespace hsk {
    MaterialBuffer::MaterialBuffer(const VkContext* context) : mBuffer(context, false)
    {
        mBuffer.GetBuffer().SetName("MaterialBuffer");
        mDescriptorBufferInfos.resize(1);
    }
    void MaterialBuffer::UpdateDeviceLocal()
    {
        if(!mBuffer.GetVector().size())
        {
            // Push something empty so we don't fail to write to the descriptor set down the line
            mBuffer.GetVector().push_back(MaterialBufferEntry{});
        }
        mBuffer.InitOrUpdate();
    }
    void MaterialBuffer::Cleanup()
    {
        mBuffer.Cleanup();
    }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MaterialBuffer::GetDescriptorInfo(VkShaderStageFlags shaderStage)
    {
        auto descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mBuffer.GetBuffer().FillVkDescriptorBufferInfo(&mDescriptorBufferInfos[0]);
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStage, &mDescriptorBufferInfos);
        return descriptorInfo;
    }

}  // namespace hsk