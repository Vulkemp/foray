#pragma once
#include "../hsk_env.hpp"
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace hsk {

    /// @brief Singleton via ShaderManager::Instance. Instance can be set by the user
    class ShaderManager
    {
      public:
        static inline ShaderManager* sInstance{nullptr};
        static ShaderManager&        Instance()
        {
            static ShaderManager sm;
            return sm;

            /*if(sInstance == nullptr)
            {
                sInstance = sDefaultInstance;
            }*/
        }
        static void SetInstance(ShaderManager* instance) { sInstance = instance; }

        /// @brief Get the shaders SpirV binary code. Automatically recompiles shader if outdated.
        /// @param shaderSourceFilePath - The path to the source code that you wish to retrieve the SpirV binary for.
        const void GetShaderBinary(std::string_view filePath, std::vector<char>& out);

        /// @brief Returns true if any shader was updated.
        /// Will check all tracked shader files for modifications and recompile the includers accordingly.
        virtual bool CheckAndUpdateShaders();

        virtual bool HasShaderBeenRecompiled(std::string_view filePath);

        /// @brief Number of recursive include lookups.
        uint32_t mMaxRecursionDepth = 10;

      protected:
        static inline ShaderManager* sDefaultInstance;
        ShaderManager(){};

        virtual bool CallGlslCompiler(std::string_view inputFilePath, std::string_view outputFilePath);

        void ScanIncludes(std::vector<Utf8Path>& outIncludes, const Utf8Path& fileToScanPath, uint32_t recursionDepth = 0);

        virtual void GetAllIncludesWithRelativePath(const std::string& inputFilePath, std::vector<std::string>* includes);

        /// @brief Returns the full file path of a relative include
        virtual Utf8Path GetIncludeFilePathRelativeToFile(const Utf8Path& includer, std::string_view includee);

        void RecompileModifiedShaders();

        virtual Utf8Path GetFileOutputPath(const Utf8Path& sourceFile)
        {
            std::string filename = (const std::string&)sourceFile + ".spv";
            return Utf8Path(filename);
        }

        std::unordered_set<Utf8Path> mNeedRecompileShaderFiles;

        std::unordered_set<Utf8Path> mRecompiledShaders;

        void CheckIncludeeForRecompilations(const std::string& includee);

        void CheckIncluderForRecompilation(const std::string& includer);

        void AddShaderIncludeesToModificationTracking(const std::string& shaderSourceFilePath);

        struct IncluderCache
        {
            IncluderCache() = default;

            /// @brief Maps includee to list of includers.
            std::unordered_map<Utf8Path, std::unordered_set<Utf8Path>> Includees;

            /// @brief After a failed compilation wait for an update.
            std::unordered_map<Utf8Path, std::filesystem::file_time_type> FailedCompileTimestamps;

            void AddTrackedIncludee(const Utf8Path& includee)
            {
                if(LookupTrackedIncludees.find(includee) == LookupTrackedIncludees.end())
                {
                    LookupTrackedIncludees.insert(includee);
                }
            }

            void AddTrackedIncluder(const Utf8Path& includer)
            {
                if(LookupTrackedIncluders.find(includer) == LookupTrackedIncluders.end())
                {
                    LookupTrackedIncluders.insert(includer);
                }
            }

            /// @brief Used for reverse lookup to find all includers of an includee.
            void AddIncludedBy(const Utf8Path& includee, const Utf8Path& includer)
            {
                if(Includees.find(includee) == Includees.end())
                {
                    Includees[includee] = {};
                }
                Includees[includee].insert(includer);
            }

            std::vector<char>& AddSpirvFile(const std::string& shaderSourceFilePath, std::string& shaderSpirvPath);

            std::set<Utf8Path> LookupTrackedIncludees;

            std::set<Utf8Path> LookupTrackedIncluders;

        } Cache;
    };

}  // namespace hsk