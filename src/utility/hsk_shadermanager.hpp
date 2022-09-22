#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
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
        static ShaderManager& SetInstance(ShaderManager* instance) { sInstance = instance; }

        /// @brief Get the shaders SpirV binary code. Automatically recompiles shader if outdated.
        /// @param shaderSourceFilePath - The path to the source code that you wish to retrieve the SpirV binary for.
        const std::vector<char>& GetShaderBinary(std::string relativeFilePath);

        const std::vector<char>& GetShaderBinaryAbsolutePath(std::string shaderSourceFilePath);

        /// @brief Returns true if any shader was updated.
        /// Will check all tracked shader files for modifications and recompile the includers accordingly.
        virtual bool CheckAndUpdateShaders();

        virtual bool HasShaderBeenRecompiledAbsolutePath(std::string& inputFilePath) { return mRecompiledShaders.find(inputFilePath) != mRecompiledShaders.end(); }

        virtual bool HasShaderBeenRecompiledRelativePath(std::string& inputFilePath);

        /// @brief Number of recursive include lookups.
        uint32_t mMaxRecursionDepth = 10;

      protected:
        static inline ShaderManager* sDefaultInstance;
        ShaderManager(){};

        virtual bool CallGlslCompiler(std::string_view inputFilePath, std::string_view outputFilePath);

        void ScanIncludes(std::vector<std::string>* outIncludes, const std::string& fileToScanPath, uint32_t recursionDepth = 0);

        virtual void GetAllIncludesWithRelativePath(const std::string& inputFilePath, std::vector<std::string>* includes);

        /// @brief Returns the full file path of a relative include
        virtual std::string GetIncludeFilePathRelativeToFile(const std::string& includer, const std::string& includee);

        /// @brief Removes all ../ redirections from the given path.
        virtual void NormalizePath(std::string& inputFilePath);

        void RecompileModifiedShaders();

        void SplitString(std::string const& str, const char delim, std::vector<std::string>& out);

        virtual std::string GetFileOutputPath(const std::string& sourceFile) { return sourceFile + ".spv"; }

        std::unordered_set<std::string_view> mNeedRecompileShaderFiles;

        std::unordered_set<std::string_view> mRecompiledShaders;

        void CheckIncludeeForRecompilations(const std::string& includee);

        void CheckIncluderForRecompilation(const std::string& includer);

        void AddShaderIncludeesToModificationTracking(const std::string& shaderSourceFilePath);

        struct IncluderCache
        {

            /// @brief Maps includee to list of includers.
            std::unordered_map<std::string, std::unordered_set<std::string>> Includees;

            /// @brief After a failed compilation wait for an update.
            std::unordered_map<std::string, std::filesystem::file_time_type> FailedCompileTimestamps;

            void AddTrackedIncludee(const std::string& includee)
            {
                if(LookupTrackedIncludees.find(includee) == LookupTrackedIncludees.end())
                {
                    LookupTrackedIncludees.insert(includee);
                    IterateTrackedIncludees.push_back(includee);
                }
            }
            
            void AddTrackedIncluder(const std::string& includer)
            {
                if(LookupTrackedIncluders.find(includer) == LookupTrackedIncluders.end())
                {
                    LookupTrackedIncluders.insert(includer);
                    IterateTrackedIncluders.push_back(includer);
                }
            }

            /// @brief Used for reverse lookup to find all includers of an includee.
            void AddIncludedBy(const std::string& includee, const std::string& includer)
            {
                if(Includees.find(includee) == Includees.end())
                {
                    Includees[includee] = {};
                }
                Includees[includee].insert(includer);
            }

            std::vector<char>& AddSpirvFile(const std::string& shaderSourceFilePath, std::string& shaderSpirvPath);

            /// @brief Returns nullptr if spirv file is not cached, otherwise a valid ptr.
            std::vector<char>* GetSpirvFileBufferPtr(std::string& shaderSourceFilePath);

            /// @brief Maps a shaderfile to its spirvcode.
            std::unordered_map<std::string, std::vector<char>> mMapSourceFilePathToSpirv;

            std::unordered_set<std::string> LookupTrackedIncludees;
            std::vector<std::string>        IterateTrackedIncludees;
            
            std::unordered_set<std::string> LookupTrackedIncluders;
            std::vector<std::string>        IterateTrackedIncluders;

        } Cache;
    };

}  // namespace hsk