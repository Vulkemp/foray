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
        std::vector<uint8_t> binary;
        ShaderManager::Instance().GetShaderBinary(relativeSpirvPath, binary);
        LoadFromBinary(context, binary); 
    }

    void ShaderModule::LoadFromSource(Context* context, std::string relativeSourcePath) {
        std::vector<uint8_t> binary;
        ShaderManager::Instance().GetShaderBinary(relativeSourcePath, binary);
        LoadFromBinary(context, binary); 
    }

    void ShaderModule::LoadFromBinary(Context* context, const std::vector<uint8_t>& binaryBuffer)
    {
        LoadFromBinary(context, reinterpret_cast<const uint32_t*>(binaryBuffer.data()), binaryBuffer.size());
    }

    void ShaderModule::LoadFromBinary(Context* context, const uint32_t* binaryBuffer, size_t sizeInBytes)
    {
        mContext = context;

        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = sizeInBytes;
        moduleCreateInfo.pCode    = binaryBuffer;
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