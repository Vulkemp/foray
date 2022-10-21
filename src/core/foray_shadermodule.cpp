#include "foray_shadermodule.hpp"
#include "../osi/foray_env.hpp"
#include "../foray_exception.hpp"
#include "foray_shadermanager.hpp"
#include <fstream>

namespace foray::core {
    ShaderModule::ShaderModule(Context* context, std::string relativeSpirvPath)
    {
        LoadFromSpirv(context, relativeSpirvPath);
    }

    void ShaderModule::LoadFromSpirv(Context* context, std::string relativeSpirvPath)
    {
        std::vector<char> binary;
        ShaderManager::Instance().GetShaderBinary(relativeSpirvPath, binary);
        LoadFromBinary(context, binary); 
    }

    void ShaderModule::LoadFromSource(Context* context, std::string relativeSourcePath) {
        std::vector<char> binary;
        ShaderManager::Instance().GetShaderBinary(relativeSourcePath, binary);
        LoadFromBinary(context, binary); 
    }

    void ShaderModule::LoadFromBinary(Context* context, std::vector<char>& binaryBuffer)
    {
        mContext                         = context;

        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = binaryBuffer.size();
        moduleCreateInfo.pCode    = (uint32_t*)binaryBuffer.data();
        mContext->VkbDispatchTable->createShaderModule(&moduleCreateInfo, NULL, &mShaderModule);
    }


    void ShaderModule::Destroy()
    {
        if(mShaderModule != nullptr)
        {
            mContext->VkbDispatchTable->destroyShaderModule(mShaderModule, nullptr);
            mShaderModule = nullptr;
        }
    }

    ShaderModule::operator VkShaderModule() const
    {
        Assert(mShaderModule, "Attempt to access VkShaderModule but shader module is nullptr");
        return mShaderModule;
    }
}  // namespace foray