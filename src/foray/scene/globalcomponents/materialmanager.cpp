#include "materialmanager.hpp"

namespace foray::scene::gcomp {
    MaterialManager::MaterialManager(core::Context* context) : mBuffer(context, false)
    {
    }
    void MaterialManager::UpdateDeviceLocal()
    {
        if(!mBuffer.GetVector().size())
        {
            // Push something empty so we don't fail to write to the descriptor set down the line
            mBuffer.GetVector().push_back(Material{});
        }
        mBuffer.InitOrUpdate();
        mBuffer.GetBuffer()->SetName("MaterialManager");
    }
}  // namespace foray