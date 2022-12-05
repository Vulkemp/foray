#include "foray_shadermodule.hpp"
#include "../foray_exception.hpp"
#include "../osi/foray_env.hpp"
#include "foray_shadermanager.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace foray::core {
    uint64_t ShaderModule::CompileFromSource(Context* context, const osi::Utf8Path& path, const ShaderCompilerConfig& config)
    {
        return context->ShaderMan->CompileShader(path, this, config, context);
    }
    void ShaderModule::LoadFromFile(Context* context, const osi::Utf8Path& path)
    {
        osi::Utf8Path absolutePath = path.IsRelative() ? path.MakeAbsolute() : path;

        std::ifstream file((fs::path)absolutePath, std::ios::binary | std::ios::in | std::ios::ate);

        FORAY_ASSERTFMT(file.is_open(), "Could not open spirv file: \"{}\"", absolutePath.GetPath());

        std::vector<uint32_t> binaryBuffer;

        size_t fileSize = (size_t)file.tellg();
        binaryBuffer.resize((fileSize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
        file.seekg(0);
        char* readPtr = reinterpret_cast<char*>(binaryBuffer.data());
        file.read(readPtr, static_cast<std::streamsize>(fileSize));
        file.close();
        LoadFromBinary(context, binaryBuffer.data(), fileSize);
    }

    void ShaderModule::LoadFromBinary(Context* context, const uint8_t* binaryBuffer, size_t sizeInBytes)
    {
        LoadFromBinary(context, reinterpret_cast<const uint32_t*>(binaryBuffer), sizeInBytes);
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

    VkPipelineShaderStageCreateInfo ShaderModule::GetShaderStageCi(VkShaderStageFlagBits stage, const char* entry) const
    {
        return
        VkPipelineShaderStageCreateInfo
        {
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage,
            .module = mShaderModule,
            .pName = entry,
            .pSpecializationInfo = nullptr,
        };
    }
}  // namespace foray::core