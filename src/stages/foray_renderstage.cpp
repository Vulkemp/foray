#include "foray_renderstage.hpp"
#include "../core/foray_shadermanager.hpp"
#include "../core/foray_shadermodule.hpp"

namespace foray::stages {

    RenderStage::RenderStage(core::Context* context, RenderDomain* domain, int32_t priority)
        : mContext(context), mDomain(domain), mOnResized(nullptr, nullptr, priority), mOnShadersRecompiled()
    {
        if(!!mDomain)
        {
            mOnResized.Set(
                domain->OnResized(), [this](VkExtent2D extent) { this->OnResized(extent); }, priority);
        }
        else
        {
            mOnResized.SetPriority(priority);
        }
        if(!!mContext && !!mContext->ShaderMan)
        {
            mOnShadersRecompiled.Set(mContext->ShaderMan->OnShadersRecompiled(), [this](const core::ShaderManager::KeySet& set) { this->OnShadersRecompiled(set); });
        }
    }

    void RenderStage::InitCallbacks(core::Context* context, RenderDomain* domain, int32_t priority)
    {
        if(!!domain)
        {
            mDomain = domain;
            mOnResized.Set(
                mDomain->OnResized(), [this](VkExtent2D extent) { this->OnResized(extent); }, priority);
        }

        if(!!context)
        {
            mContext = context;
            if(!!mContext->ShaderMan)
            {
                mOnShadersRecompiled.Set(mContext->ShaderMan->OnShadersRecompiled(), [this](const core::ShaderManager::KeySet& set) { this->OnShadersRecompiled(set); });
            }
        }
    }

    void RenderStage::SetResizeOrder(int32_t priority)
    {
        mOnResized.SetPriority(priority);
    }

    void RenderStage::Destroy()
    {
        mOnResized.Destroy();
        mOnShadersRecompiled.Destroy();
    }

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
    void RenderStage::OnResized(VkExtent2D extent)
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
        bool needReload = false;

        for(uint64_t key : mShaderKeys)
        {
            needReload |= recompiled.contains(key);
        }

        if(needReload)
        {
            ReloadShaders();
        }
    }
}  // namespace foray::stages