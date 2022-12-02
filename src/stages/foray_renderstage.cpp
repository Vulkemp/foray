#include "foray_renderstage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"

namespace foray::stages {
    std::vector<core::ManagedImage*> RenderStage::GetImageOutputs()
    {
        std::vector<core::ManagedImage*> result;
        result.reserve(mImageOutputs.size());
        for(auto& output : mImageOutputs)
        {
            result.push_back(output.second);
        }
        return result;
    }
    core::ManagedImage* RenderStage::GetImageOutput(const std::string_view name, bool noThrow)
    {
        std::string namecopy(name);
        auto        iter = mImageOutputs.find(namecopy);
        if(iter != mImageOutputs.end())
        {
            return iter->second;
        }
        if(!noThrow)
        {
            FORAY_THROWFMT("Failed to get color attachment with name: {}", name)
        }
        return nullptr;
    }
    void RenderStage::Resize(const VkExtent2D& extent)
    {
        for(auto& pair : mImageOutputs)
        {
            if(pair.second->Exists())
            {
                pair.second->Resize(extent);
            }
        }
    }
    void RenderStage::DestroyOutputImages()
    {
        for(auto& pair : mImageOutputs)
        {
            pair.second->Destroy();
        }
        mImageOutputs.clear();
    }
    void RenderStage::OnShadersRecompiled(const std::unordered_set<uint64_t>& recompiled)
    {
        // core::ShaderManager& instance = core::ShaderManager::Instance();

        bool needReload = false;

        for(uint64_t key : mShaderKeys)
        {
            needReload |= recompiled.contains(key);
        }
        
        if (needReload)
        {
            ReloadShaders();
        }
    }
}  // namespace foray::stages