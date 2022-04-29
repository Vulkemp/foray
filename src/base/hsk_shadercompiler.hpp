#pragma once
#include <chrono>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hsk {

#ifdef _WIN32
    using SPV_STR = std::wstring;
#else
    using SPV_STR = std::string;
#endif

    namespace fs = std::filesystem;

    class ShaderCompiler
    {
      public:
        struct ShaderFileInfo
        {
            fs::path mSourcePathFull{};
            fs::path mOutPathFull{};
        };
        enum VerbosityLevel : std::uint8_t
        {
            Modified = 0x1,
            Unsupported = 0x2,
            All         = 0xff,
        };


      public:
        ShaderCompiler(){};
        void SetVerbose(VerbosityLevel verbosityLevel) { mVerbosityLevel = verbosityLevel; }
        void SetThrowException(bool throwException) { mThrowException = throwException; }

        void Init(const std::string& sourceDir, const std::string& outDir)
        {
#ifdef _WIN32
            mSourceDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(sourceDir);
            mOutputDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(outDir);
#else
            mSourceDir = sourceDir;
            mOutputDir = outDir;
#endif  // _WIN32
        }


        bool CallGlslCompiler(const ShaderFileInfo& shaderFileInfo);
        bool CompileAll();
        bool CompileShaderFile(fs::path shaderFilePath);

        /// @brief Allows manipulation of the valid file endings that the shader compiler will recognize as shader files.
        /// @return The reference to the vector to manipulate.
        std::vector<SPV_STR>& GetValidFileEndings();

      protected:
        SPV_STR mSourceDir = SPV_STR();
        SPV_STR mOutputDir = SPV_STR();
        /// @brief The verbosity level determines the shader compiler out. By default, compiled and skipped files will be announced.
        VerbosityLevel mVerbosityLevel = VerbosityLevel::Modified;
        /// @brief If true, throw an exception when a shaderfile is missing.
        bool mThrowException = true;

        /// @brief File endings that the shader compiler attempts to compile into spirv bins.
        std::vector<SPV_STR> mValidFileEndings = {
#ifdef _WIN32
            L".frag", L".vert", L".rchit", L".rmiss", L".rgen", L".comp",
#else
            ".frag", ".vert", ".rchit", ".rmiss", ".rgen", ".comp",
#endif
        };

        bool IsValidSourceFile(const fs::path& shaderFilePath);
        bool NeedsCompiling(const ShaderFileInfo& shaderFile);
        void LogLastModified(const ShaderFileInfo& shaderFile);

        // https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c
        bool EndsWith(const SPV_STR& str, const SPV_STR& suffix);

        bool StartsWith(const SPV_STR& str, const SPV_STR& prefix);

        std::string FileTimeToString(fs::file_time_type filetime);
    };

}  // namespace hsk