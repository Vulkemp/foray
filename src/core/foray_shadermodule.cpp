#include "foray_shadermodule.hpp"
#include "../osi/foray_env.hpp"
#include "../foray_exception.hpp"
#include "foray_shadermanager.hpp"
#include <fstream>

namespace foray::core {
    ShaderModule::ShaderModule(const VkContext* context, std::string relativeSpirvPath)
    {
        LoadFromSpirv(context, relativeSpirvPath);
    }

    void ShaderModule::LoadFromSpirv(const VkContext* context, std::string relativeSpirvPath)
    {
        std::vector<char> binary;
        ShaderManager::Instance().GetShaderBinary(relativeSpirvPath, binary);
        LoadFromBinary(context, binary); 
    }

    void ShaderModule::LoadFromSource(const VkContext* context, std::string relativeSourcePath) {
        std::vector<char> binary;
        ShaderManager::Instance().GetShaderBinary(relativeSourcePath, binary);
        LoadFromBinary(context, binary); 
    }

    void ShaderModule::LoadFromBinary(const VkContext* context, std::vector<char>& binaryBuffer)
    {
        mContext                         = context;

        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = binaryBuffer.size();
        moduleCreateInfo.pCode    = (uint32_t*)binaryBuffer.data();
        vkCreateShaderModule(context->Device, &moduleCreateInfo, NULL, &mShaderModule);
    }


    void ShaderModule::Destroy()
    {
        if(mShaderModule != nullptr)
        {
            vkDestroyShaderModule(mContext->Device, mShaderModule, nullptr);
            mShaderModule = nullptr;
        }
    }

    ShaderModule::operator VkShaderModule() const
    {
        Assert(mShaderModule, "Attempt to access VkShaderModule but shader module is nullptr");
        return mShaderModule;
    }
}  // namespace foray