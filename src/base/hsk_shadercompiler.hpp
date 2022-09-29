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
        enum VerbosityFlags : std::uint8_t
        {
            Modified   = (1 << 1),  // log if a file was modified
            Unmodified = (1 << 2),  // log if a file was not modified
            Verbose    = (1 << 3),  // log miscellaneous verbose output
            All        = 0xff,
        };


      public:
        ShaderCompiler(){};
        void SetVerbosityFlags(VerbosityFlags verbosityLevel) { mVerbosityFlags = verbosityLevel; }
        void SetThrowException(bool throwException) { mThrowException = throwException; }

        /// @brief Adding a source directory to search recursivly for shader files.
        /// @param sourceDirectory
        void AddSourceDirectory(const std::string& sourceDirectory);

        /// @brief Tells the shader compiler to output all shader files to this directory.
        /// If no directory is specified, .spv outputs are placed next to their sources.
        void SetOutputDirectory(const std::string& outputDirectory);

        bool CallGlslCompiler(const ShaderFileInfo& shaderFileInfo);
        bool CompileAll(bool recompile = false);

        /// @brief Expects a glsl shader file and generates a .spv in the same folder.
        /// The path is made of 3 parts. The path to the repository, the relativeSourceDir in the repository &
        /// the path to the shader file in the source directory.
        /// @param relativeSourceDir -
        /// @param shaderFilePath -
        /// @param recompile - Indicates this is a recompilation, aka. a compilation that happens after initial program start.
        /// @return
        bool CompileShaderFile(fs::path relativeSourceDir, fs::path shaderFilePath, bool recompile = false);

        /// @brief Allows manipulation of the valid file endings that the shader compiler will recognize as shader files.
        /// @return The reference to the vector to manipulate.
        std::vector<SPV_STR>& GetValidFileEndings();

        /// @brief ShaderCompiler intern logic for shader recompilation tracking. Feel free to implement one on your own for more
        /// complex use cases. Supports only unique shader file names.
        /// Returns true if the shader has been recompiled since the last time this method was called for its filename.
        /// @param shaderFilePath - Shaderfile to check for recompilation. Only the filename is used for comparison and must be unique.
        /// @param invalidate - If true, the recompilation map will be updated, so this method returns false for this shader file
        /// until the next recompilation occurs.
        bool HasShaderBeenRecompiled(std::string& shaderFilePath, bool invalidate = true);

      protected:
        SPV_STR mOutputDir = SPV_STR();
        /// @brief The verbosity level determines the shader compiler out. By default, compiled and skipped files will be announced.
        VerbosityFlags mVerbosityFlags = (VerbosityFlags)(VerbosityFlags::Modified | VerbosityFlags::Unmodified);
        /// @brief If true, throw an exception when a shaderfile is missing.
        bool mThrowException = true;

        std::unordered_map<std::string, bool> mMapShadersRecompiled;

        std::vector<SPV_STR> mSourceDirectories;

        /// @brief File endings that the shader compiler attempts to compile into spirv bins.
        std::vector<SPV_STR> mValidFileEndings = {
#ifdef _WIN32
            L".frag", L".vert", L".rchit", L".rahit", L".rmiss", L".rgen", L".comp",
#else
            ".frag", ".vert", ".rchit", ".rahit", ".rmiss", ".rgen", ".comp",
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