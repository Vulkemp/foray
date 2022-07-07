#include "hsk_materialbuffer.hpp"

namespace hsk {
    MaterialBuffer::MaterialBuffer(const VkContext* context) : mBuffer(context, false)
    {
        mBuffer.GetBuffer().SetName("MaterialBuffer");
        mDescriptorBufferInfos.resize(1);
    }
    void MaterialBuffer::UpdateDeviceLocal() { mBuffer.InitOrUpdate(); }
    void MaterialBuffer::Cleanup() { mBuffer.Cleanup(); }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MaterialBuffer::GetDescriptorInfo()
    {
        if(mDescriptorInfo != nullptr)
        {
            return mDescriptorInfo;
        }

        mDescriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        mBuffer.GetBuffer().FillVkDescriptorBufferInfo(&mDescriptorBufferInfos[0]);
        mDescriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, &mDescriptorBufferInfos);
        return mDescriptorInfo;
    }

}  // namespace hsk