#include "hsk_materialbuffer.hpp"

namespace hsk {
    NMaterialBuffer::NMaterialBuffer(const VkContext* context) : mBuffer(context, false) { mBuffer.GetBuffer().SetName("MaterialBuffer"); }
    void NMaterialBuffer::UpdateDeviceLocal() { mBuffer.InitOrUpdate(); }
    void NMaterialBuffer::Cleanup() { mBuffer.Cleanup(); }

    std::shared_ptr<DescriptorSetHelper::DescriptorInfo> NMaterialBuffer::MakeDescriptorInfo()
    {
        auto                                descriptorInfo = std::make_shared<DescriptorSetHelper::DescriptorInfo>();
        std::vector<VkDescriptorBufferInfo> bufferInfos    = {mBuffer.GetBuffer().GetVkDescriptorBufferInfo()};
        descriptorInfo->Init(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfos);
        return descriptorInfo;
    }

}  // namespace hsk