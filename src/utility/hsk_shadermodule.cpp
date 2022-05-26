#include "hsk_shadermodule.hpp"
#include "../hsk_env.hpp"
#include "../hsk_exception.hpp"
#include "../hsk_vkHelpers.hpp"

namespace hsk {
    ShaderModule::ShaderModule(const VkContext* context, std::string relativeSpirvPath) { LoadFromSpirv(context, relativeSpirvPath); }

    void ShaderModule::LoadFromSpirv(const VkContext* context, std::string relativeSpirvPath)
    {
        mContext                         = context;
        auto          mAbsoluteSpirvPath = MakeRelativePath(relativeSpirvPath);
        std::ifstream is(mAbsoluteSpirvPath.c_str(), std::ios::binary | std::ios::in | std::ios::ate);

        if(!is.is_open())
        {
            throw Exception("Could not open shader file: {}", mAbsoluteSpirvPath);
        }

        size_t size = is.tellg();
        is.seekg(0, std::ios::beg);
        char* shaderCode = new char[size];
        is.read(shaderCode, size);
        is.close();

        assert(size > 0);

        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode    = (uint32_t*)shaderCode;

        vkCreateShaderModule(context->Device, &moduleCreateInfo, NULL, &mShaderModule);

        delete[] shaderCode;
    }


    void ShaderModule::DeleteVkModule()
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
}  // namespace hsk