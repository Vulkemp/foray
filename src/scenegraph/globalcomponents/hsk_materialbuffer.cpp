#include "hsk_materialbuffer.hpp"

namespace hsk {
    MaterialBuffer::MaterialBuffer(const VkContext* context) : mBuffer(context, false) { mBuffer.GetBuffer().SetName("MaterialBuffer"); }
    void MaterialBuffer::UpdateDeviceLocal() { mBuffer.InitOrUpdate(); }
    void MaterialBuffer::Cleanup() { mBuffer.Cleanup(); }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> MaterialBuffer::MakeDescriptorInfo()
    {
        auto                                descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        std::vector<VkDescriptorBufferInfo> bufferInfos    = {mBuffer.GetBuffer().GetVkDescriptorBufferInfo()};
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfos);
        return descriptorInfo;
    }

}  // namespace hsk