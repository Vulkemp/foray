#include "foray_materialbuffer.hpp"

namespace foray::scene {
    MaterialBuffer::MaterialBuffer(core::Context* context) : mBuffer(context, false)
    {
        mBuffer.GetBuffer().SetName("MaterialBuffer");
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
}  // namespace foray