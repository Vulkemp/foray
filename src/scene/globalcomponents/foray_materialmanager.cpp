#include "foray_materialmanager.hpp"

namespace foray::scene::gcomp {
    MaterialManager::MaterialManager(core::Context* context) : mBuffer(context, false)
    {
        mBuffer.GetBuffer().SetName("MaterialManager");
    }
    void MaterialManager::UpdateDeviceLocal()
    {
        if(!mBuffer.GetVector().size())
        {
            // Push something empty so we don't fail to write to the descriptor set down the line
            mBuffer.GetVector().push_back(DefaultMaterialEntry{});
        }
        mBuffer.InitOrUpdate();
    }
    void MaterialManager::Destroy()
    {
        mBuffer.Destroy();
    }
}  // namespace foray