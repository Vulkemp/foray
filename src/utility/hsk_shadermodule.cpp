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
        std::ifstream file(mAbsoluteSpirvPath.c_str(), std::ios::binary | std::ios::in | std::ios::ate);

        if(!file.is_open())
        {
            throw Exception("Could not open shader file: {}", mAbsoluteSpirvPath);
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();

        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = buffer.size();
        moduleCreateInfo.pCode    = (uint32_t*)buffer.data();
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
}  // namespace hsk