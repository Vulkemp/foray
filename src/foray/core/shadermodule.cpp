#include "shadermodule.hpp"
#include "../exception.hpp"
#include "../osi/path.hpp"
#include "shadermanager.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace foray::core {
    ShaderModule::ShaderModule(Context* context, const osi::Utf8Path& spirvFilePath)
     : mContext(context)
    {
        osi::Utf8Path absolutePath = spirvFilePath.IsRelative() ? spirvFilePath.MakeAbsolute() : spirvFilePath;

        std::ifstream file((fs::path)absolutePath, std::ios::binary | std::ios::in | std::ios::ate);

        FORAY_ASSERTFMT(file.is_open(), "Could not open spirv file: \"{}\"", absolutePath.GetPath());

        std::vector<uint32_t> binaryBuffer;

        size_t fileSize = (size_t)file.tellg();
        binaryBuffer.resize((fileSize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
        file.seekg(0);
        char* readPtr = reinterpret_cast<char*>(binaryBuffer.data());
        file.read(readPtr, static_cast<std::streamsize>(fileSize));
        file.close();

        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = binaryBuffer.size() * sizeof(uint32_t);
        moduleCreateInfo.pCode    = binaryBuffer.data();
        AssertVkResult(mContext->DispatchTable().createShaderModule(&moduleCreateInfo, NULL, &mShaderModule));
    }

    ShaderModule::ShaderModule(Context* context, const uint32_t* spirvBinary, size_t size) : mContext(context)
    {
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode    = spirvBinary;
        AssertVkResult(mContext->DispatchTable().createShaderModule(&moduleCreateInfo, NULL, &mShaderModule));
    }

    ShaderModule::ShaderModule(Context* context, std::span<const uint32_t> spirvBinary) 
     : ShaderModule(context, spirvBinary.data(), spirvBinary.size() * sizeof(uint32_t))
    {
    }

    ShaderModule::~ShaderModule()
    {
        if(mShaderModule != nullptr)
        {
            mContext->DispatchTable().destroyShaderModule(mShaderModule, nullptr);
            mShaderModule = nullptr;
        }
    }

    ShaderModule::operator vk::ShaderModule() const
    {
        Assert(mShaderModule, "Attempt to access vk::ShaderModule but shader module is nullptr");
        return mShaderModule;
    }

    VkPipelineShaderStageCreateInfo ShaderModule::GetShaderStageCi(vk::ShaderStageFlagBits stage, const char* entry) const
    {
        return VkPipelineShaderStageCreateInfo{
            .sType               = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = stage,
            .module              = mShaderModule,
            .pName               = entry,
            .pSpecializationInfo = nullptr,
        };
    }
}  // namespace foray::core