#include "hsk_materialbuffer.hpp"

namespace hsk {
    NMaterialBuffer::NMaterialBuffer(const VkContext* context) : mBuffer(context, false) { mBuffer.GetBuffer().SetName("MaterialBuffer"); }
    void NMaterialBuffer::UpdateDeviceLocal() { mBuffer.InitOrUpdate(); }
    void NMaterialBuffer::Cleanup() { mBuffer.Cleanup(); }
}  // namespace hsk