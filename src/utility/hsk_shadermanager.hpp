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

        /// @brief Returns true if any of the tracked files have been changed and some shaders need recompilation.
        /// Should trigger a shaders recompiled event.
        virtual bool CheckTrackedFilesForModification();

        virtual void RecompileModifiedShaders();

        virtual bool HasShaderBeenRecompiledAbsolutePath(std::string& inputFilePath) { return mNeedRecompileShaderFiles.find(inputFilePath) != mNeedRecompileShaderFiles.end(); }

        virtual bool HasShaderBeenRecompiledRelativePath(std::string& inputFilePath);

        /// @brief Number of recursive include lookups.
        uint32_t mMaxRecursionDepth = 10;

      protected:

        static inline ShaderManager* sDefaultInstance;
        ShaderManager(){};

        virtual bool CallGlslCompiler(const std::string& inputFilePath, const std::string& outputFilePath);

        /// @brief Scan file for include directives and returns them in the includes vector.
        virtual bool RecursiveScanIncludes(std::string& inputFilePath, uint32_t recursionDepth = 0);

        virtual void GetAllIncludes(std::string& inputFilePath, std::vector<std::string>* includes);

        /// @brief Returns the full file path of a relative include
        virtual std::string GetIncludeFilePathRelativeToFile(std::string& includer, const std::string& includee);

        /// @brief Removes all ../ redirections from the given path.
        virtual void NormalizePath(std::string& inputFilePath);

        void SplitString(std::string const& str, const char delim, std::vector<std::string>& out);

        virtual std::string GetFileOutputPath(const std::string& sourceFile) { return sourceFile + ".spv"; }

        std::unordered_set<std::string> mNeedRecompileShaderFiles;

        virtual void AddSpirvFile(std::string& shaderSpirvFullPath);

        /// @brief Maps a shaderfile to its spirvcode.
        std::unordered_map<std::string, std::vector<char>> mSpirvCodeMap;

        struct IncluderCache
        {
            // Allows to map an includee the current main includer.
            std::string CurrentMainIncluder;

            /// @brief Tracked files will be checked for modification every modification check update. Lookup if file added.
            std::unordered_set<std::string> TrackedFilesLookup;

            /// @brief Used to quickly iterate all tracked files.
            std::vector<std::string> TrackedFilesIterate;

            /// @brief Maps includee to list of includers.
            std::unordered_map<std::string, std::unordered_set<std::string>> Includees;

            void AddMainFileToIncludee(std::string& includee)
            {
                if(Includees.find(includee) == Includees.end())
                {
                    Includees[includee] = {};
                }
                Includees[includee].insert(CurrentMainIncluder);
            }
        } Cache;
    };

}  // namespace hsk