#pragma once
#include "../osi/foray_env.hpp"
#include "foray_core_declares.hpp"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace foray::core {

    /// @brief Shader manager maintains a structure of shader compilations
    /// @details 
    /// When registering a shader file compilation via CompileShader(..):
    ///  - The shader compilation action is saved and tracked via a key
    ///  - Any changes to the shader source file or files included via #include directives in the shader (recursively resolved) cause attempt of recompiling the shader.
    ///  - CheckAndUpdate function checks files as described and returns set of successfully recompiled shader compilation keys.
    class ShaderManager
    {
      public:
        /// @param context If set, is used as fallback for CompileShader() context argument
        inline explicit ShaderManager(core::Context* context = nullptr) : mContext(context) {}

        /// @brief Options struct
        struct CompileConfig
        {
            /// @brief Additional include dirs passed to glslc via "-I "INCLUDEDIR"" option
            std::vector<osi::Utf8Path> IncludeDirs = {};
            /// @brief Preprocessor definitions passed to glslc via "-DDEFINITION" option
            std::vector<std::string> Definitions = {};
            /// @brief Shader Stage entry point passed to gslc via "-fentry-point=NAME". Leave empty to let glslc use default ("main")
            std::string EntryPoint = "";
            /// @brief Options in this vector are appended to the option string as-is
            std::vector<std::string> AdditionalOptions = {};
        };

        /// @brief Accesses sourceFilePath, scans for dependencies, compiles if necessary, loads into shaderModule
        /// @param sourceFilePath Path to the shader source file
        /// @param shaderModule Output shadermodule to initialize
        /// @param config Configure compilation parameters
        /// @param context Context for initialization of ShaderModule. If set to nullptr, will use ShaderManager context
        /// @return Returns the key to this unique shader compilation
        uint64_t CompileShader(osi::Utf8Path sourceFilePath, ShaderModule& shaderModule, const CompileConfig& config = CompileConfig(), core::Context* context = nullptr);
        /// @brief Accesses sourceFilePath, scans for dependencies, compiles if necessary, loads into shaderModule
        /// @param sourceFilePath Path to the shader source file
        /// @param shaderModule Output shadermodule to initialize
        /// @param config Configure compilation parameters
        /// @param context Context for initialization of ShaderModule. If set to nullptr, will use ShaderManager context
        /// @return Returns the key to this unique shader compilation
        uint64_t CompileShader(osi::Utf8Path sourceFilePath, ShaderModule* shaderModule, const CompileConfig& config = CompileConfig(), core::Context* context = nullptr);

        /// @brief Checks and updates shader compilations for source code changes
        /// @details Will check all tracked shader files for modifications and recompile the shader compilations accordingly
        /// @param out_recompiled Output set for shader compilation keys which have been recompiled
        /// @return Returns true if any shader was updated
        virtual bool CheckAndUpdateShaders(std::unordered_set<uint64_t>& out_recompiled);

        ShaderManager() = default;

        /// @brief Calls glslc in path on linux, glslc.exe (derived from VULKAN_SDK environment variable) on windows
        /// @param args Arguments are passed as follows: GLSLC_EXECUTABLE --target-spv=spv1.5 OPTIMIZE ARGS
        /// (OPTIMIZE = -O0 for DEBUG, -O for RELEASE CMake targets)
        /// @return True if glslc compiler exe returns 0 (indicating success), false otherwise
        virtual bool CallGlslCompiler(std::string_view args);

      protected:
        /// @brief maps files to last write times
        using WriteTimeLookup = std::unordered_map<osi::Utf8Path, std::filesystem::file_time_type>;

        /// @brief Context used for initialization of shader modules
        core::Context* mContext = nullptr;

        /// @brief Calculates a unique hash based on source file path and config
        virtual uint64_t MakeHash(std::string_view absoluteUniqueSourceFilePath, const CompileConfig& config);

        /// @brief Get last write time
        /// @param path File path
        /// @param writeTimeLookup Lookup
        /// @return Lookup table result / std::filesystem::file_time_type::min() if not found, last write time otherwise
        static std::filesystem::file_time_type GetWriteTime(const osi::Utf8Path& path, WriteTimeLookup& writeTimeLookup);

        struct IncludeFile;

        enum class ECompileCheckResult
        {
            UpToDate,
            MissingInput,
            NeedsRecompile
        };

        /// @brief Represents a unique shader compilation (input source path + config => Spirv file)
        struct ShaderCompilation
        {
            /// @brief Owning manager object
            ShaderManager* Manager = nullptr;
            /// @brief Source path
            osi::Utf8Path SourcePath = {};
            /// @brief Output path
            osi::Utf8Path SpvPath = {};
            /// @brief Last successful compile time
            std::filesystem::file_time_type LastCompile = std::filesystem::file_time_type::min();
            /// @brief Time of last failed compile
            std::filesystem::file_time_type LastFailedCompile = std::filesystem::file_time_type::min();
            /// @brief List of includes
            std::unordered_set<IncludeFile*> Includes = {};
            /// @brief Configuration of the shader compiler
            CompileConfig Config = {};
            /// @brief Hash identifying the compilation
            uint64_t Hash = 0;
            /// @brief Sets SpvPath and updates LastCompile
            /// @param source Absolute source file path
            /// @param config Compiler configuration
            /// @param hash compilation hash
            ShaderCompilation(ShaderManager* manager, const osi::Utf8Path& source, const CompileConfig& config, uint64_t hash);
            /// @brief Finds all includes and configures them in the watch of the shader manager
            void FindIncludes();
            /// @brief Invokes the shader compiler
            /// @return True, if compilation was successful
            bool Compile();
            /// @brief Checks for need to recompile
            /// @param compileTimeLookup Lookup for writetimes of already checked files
            ECompileCheckResult NeedsCompile(WriteTimeLookup& compileTimeLookup);
        };

        /// @brief Represents a file included (nested) in a shader compilation, but not compiled by itself
        struct IncludeFile
        {
            /// @brief File path
            osi::Utf8Path Path;
            /// @brief Compilations depending on this file
            std::unordered_set<ShaderCompilation*> Includers;
            /// @brief Files included by this include file
            std::unordered_set<IncludeFile*> Includees;

            inline IncludeFile(const osi::Utf8Path& path) : Path(path) {}

            /// @brief Recursively writes Includees to out
            void GetIncludesRecursively(std::unordered_set<IncludeFile*>& out);
        };

        /// @brief Map of shader compilation keys to tracked compilations
        std::unordered_map<uint64_t, std::unique_ptr<ShaderCompilation>> mTrackedCompilations;
        /// @brief Map of include file paths to include file structs
        std::unordered_map<osi::Utf8Path, std::unique_ptr<IncludeFile>> mTrackedIncludeFiles;

        /// @brief Discover and register all includes for a shader compilation
        void DiscoverIncludes(ShaderCompilation* compilation);
        /// @brief Create or return an include file. Recursively processes #include directives to resolve nested includes
        /// @param path Absolute file path
        /// @param extraIncludeDirs Additional include dirs
        /// @return Include File
        IncludeFile* RecursivelyProcessInclude(const osi::Utf8Path& path, const std::vector<osi::Utf8Path>& extraIncludeDirs);
    };

}  // namespace foray::core