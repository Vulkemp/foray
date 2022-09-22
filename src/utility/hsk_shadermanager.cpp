#include "hsk_shadermanager.hpp"
#include "../hsk_env.hpp"
#include <regex>

namespace hsk {

    const void ShaderManager::GetShaderBinary(std::string_view filePath, std::vector<char>& out)
    {
        Utf8Path sourceFilePath(filePath);
        if(sourceFilePath.IsRelative())
        {
            sourceFilePath = sourceFilePath.MakeAbsolute();
        }


        if(Cache.LookupTrackedIncluders.find(sourceFilePath) == Cache.LookupTrackedIncluders.end())
        {
            AddShaderIncludeesToModificationTracking(sourceFilePath);

            // add includers to tracking
            Cache.AddTrackedIncluder(sourceFilePath);

            CheckAndUpdateShaders();
        }

        Utf8Path outputFilePath = GetFileOutputPath(sourceFilePath);

        std::ifstream file(outputFilePath.GetPath(), std::ios::binary | std::ios::in | std::ios::ate);

        HSK_ASSERTFMT(file.is_open(), "Could not open shader file: {}", outputFilePath.GetPath());

        size_t fileSize = (size_t)file.tellg();
        out.resize(fileSize);
        file.seekg(0);
        file.read(out.data(), static_cast<std::streamsize>(fileSize));
        file.close();
    }

    // const std::vector<char>& ShaderManager::GetShaderBinaryAbsolutePath(std::string shaderSourceFilePath)
    // {
    //     // clean shader file path (remove ../ etc. from path)
    //     NormalizePath(shaderSourceFilePath);
    //     std::string outputFilePath = GetFileOutputPath(shaderSourceFilePath);

    //     // if shader code cached, immediate return
    //     std::vector<char>* cachedShaderCode = Cache.GetSpirvFileBufferPtr(shaderSourceFilePath);
    //     if(cachedShaderCode != nullptr)
    //     {
    //         return *cachedShaderCode;
    //     }

    //     AddShaderIncludeesToModificationTracking(shaderSourceFilePath);

    //     // add includers to tracking
    //     Cache.AddTrackedIncluder(shaderSourceFilePath);

    //     CheckAndUpdateShaders();

    //     return Cache.AddSpirvFile(shaderSourceFilePath, outputFilePath);
    // }

#ifdef _WIN32
    // calls the glslc.exe on windows and passes the shader file path
    // returns false if the compilation failed
    bool ShaderManager::CallGlslCompiler(std::string_view inputFilePath, std::string_view outputFilePath)
    {
        // convert paths to wstring
        std::wstring inputFilePathW  = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string(inputFilePath));
        std::wstring outputFilePathW = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string(outputFilePath));
        // query vulkan sdk path
        auto         pathVariable      = std::string(std::getenv("VULKAN_SDK"));
        std::wstring pathVariableWStr  = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(pathVariable);
        std::wstring executable        = (pathVariableWStr + L"/Bin/glslc.exe");
        LPCWSTR      lpApplicationName = executable.c_str();
        //SPIRV_COMPILER_CMD_NAME_W;
        if(lpApplicationName == nullptr || (int)lpApplicationName[0] == 0)
        {
            return false;
        }
        // additional information
        STARTUPINFOW        si;
        PROCESS_INFORMATION pi;

        // set the size of the structures
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        std::wstring commandLine = std::wstring(lpApplicationName) + L" --target-spv=spv1.5 " + inputFilePathW + L" -o " + outputFilePathW;
        // start the program up
        CreateProcessW(lpApplicationName,            // the path
                       (LPWSTR)commandLine.c_str(),  // Command line
                       NULL,                         // Process handle not inheritable
                       NULL,                         // Thread handle not inheritable
                       FALSE,                        // Set handle inheritance to FALSE
                       0,                            // No creation flags
                       NULL,                         // Use parent's environment block
                       NULL,                         // Use parent's starting directory
                       &si,                          // Pointer to STARTUPINFO structure
                       &pi                           // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
        );

        // wait for compilation to finish
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode{0};
        GetExitCodeProcess(pi.hProcess, &exitCode);

        // Close process and thread handles.
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        // returns true if an error occurs
        return exitCode == 0;
    }
#else
    // returns false if compilation fails
    bool ShaderManager::CallGlslCompiler(std::string_view inputFilePath, std::string_view outputFilePath)
    {
        std::stringstream strbuilder;
        strbuilder << "/bin/glslc --target-spv=spv1.5 " << inputFilePath << " -o " << outputFilePath;
        std::string command     = strbuilder.str();
        int         returnvalue = std::system(command.c_str());
        return returnvalue == 0;
    }
#endif

    bool ShaderManager::HasShaderBeenRecompiled(std::string_view filePath)
    {
        if (filePath.empty())
        {
            return false;
        }
        Utf8Path sourceFilePath(filePath);
        if(sourceFilePath.IsRelative())
        {
            sourceFilePath = sourceFilePath.MakeAbsolute();
        }

        return mRecompiledShaders.find(sourceFilePath) != mRecompiledShaders.end();
    }

    void ShaderManager::ScanIncludes(std::vector<Utf8Path>& outIncludes, const Utf8Path& fileToScanPath, uint32_t recursionDepth)
    {
        if(recursionDepth > mMaxRecursionDepth)
        {
            throw Exception("ShaderManager max recursion depth reached. Your shader files might have circular includes.");
        }

        std::vector<std::string> localFileIncludes;
        GetAllIncludesWithRelativePath(fileToScanPath, &localFileIncludes);

        for(const std::string& includee : localFileIncludes)
        {
            // get includee path
            Utf8Path includeeFullPath = GetIncludeFilePathRelativeToFile(fileToScanPath, includee);

            // add full path to output
            outIncludes.push_back(includeeFullPath);

            // get includee's includes
            ScanIncludes(outIncludes, includeeFullPath, ++recursionDepth);
        }
    }

    Utf8Path ShaderManager::GetIncludeFilePathRelativeToFile(const Utf8Path& includer, std::string_view includee)
    {
        // remove file from includer path and append include
        std::filesystem::path includerPath(includer);
        includerPath.remove_filename();
        includerPath.append(includee);

        Utf8Path newPath(includerPath.string());
        return newPath;
    }

    bool ShaderManager::CheckAndUpdateShaders()
    {
        mNeedRecompileShaderFiles.clear();
        // check all includees if their includers need to be recompiled
        for(const Utf8Path& includee : Cache.LookupTrackedIncludees)
        {
            CheckIncludeeForRecompilations(includee);
        }

        // check includers
        for(const Utf8Path& includer : Cache.LookupTrackedIncluders)
        {
            // check if recompilation failed
            CheckIncluderForRecompilation(includer);
        }

        if(mNeedRecompileShaderFiles.size() > 0)
        {
            RecompileModifiedShaders();
            return true;
        }
        return false;
    }

    void ShaderManager::AddShaderIncludeesToModificationTracking(const std::string& shaderSourceFilePath)
    {
        std::vector<Utf8Path> includes;
        ScanIncludes(includes, shaderSourceFilePath);

        // add all includes to tracking and relate includer & includee
        for(const Utf8Path& includee : includes)
        {
            Cache.AddTrackedIncludee(includee);
            Cache.AddIncludedBy(includee, shaderSourceFilePath);
        }
    }

    void ShaderManager::RecompileModifiedShaders()
    {
        mRecompiledShaders.clear();

        for(const Utf8Path& shaderPath : mNeedRecompileShaderFiles)
        {
            // log magenta shader compiled started
            logger()->info("\033[1;35m Starting compilation of {} \033[0m", shaderPath);

            // trigger shader recompilation
            auto s       = GetFileOutputPath(std::string(shaderPath));
            bool success = CallGlslCompiler(shaderPath.GetPath(), s.GetPath());

            if(!success)
            {
                // log red failed
                logger()->error("\033[1;31m Compilation failed {} \033[0m \n", shaderPath);
                Cache.FailedCompileTimestamps[std::string(shaderPath)] = fs::last_write_time(shaderPath);
            }
            else
            {
                // log green success
                logger()->info("\033[1;32m Compilation succeeded {} \033[0m \n", shaderPath);

                // add to compiled shaders
                mRecompiledShaders.insert(shaderPath);

                // remove from compile failed cache
                if(Cache.FailedCompileTimestamps.find(shaderPath) != Cache.FailedCompileTimestamps.end())
                {
                    Cache.FailedCompileTimestamps.erase(std::string(shaderPath));
                }
            }
        }
    }

    void ShaderManager::GetAllIncludesWithRelativePath(const std::string& inputFilePath, std::vector<std::string>* includes)
    {
        std::ifstream     ifstream(inputFilePath);
        std::stringstream buffer;
        buffer << ifstream.rdbuf();
        auto        content    = buffer.str();
        std::size_t fileLength = content.length();
        std::size_t currentPos = 0;
        std::string searchStr("#include");
        std::string sep("\"");

        std::size_t firstOccurance, secondOccurance;
        while((currentPos = content.find(searchStr, currentPos)) < fileLength)
        {
            firstOccurance  = content.find(sep, currentPos);
            secondOccurance = content.find(sep, firstOccurance + 1);
            includes->push_back(content.substr(firstOccurance + 1, secondOccurance - firstOccurance - 1));
            currentPos = secondOccurance + 1;
        }
    }

    void ShaderManager::CheckIncludeeForRecompilations(const std::string& includee)
    {

        for(const std::string& includer : Cache.Includees[includee])
        {
            // check if includer recompilation failed
            if(Cache.FailedCompileTimestamps.find(includer) != Cache.FailedCompileTimestamps.end())
            {
                // if so, check if includee has been modified afterwards
                if(fs::last_write_time(includee) > Cache.FailedCompileTimestamps[includer])
                {
                    // if yes, its time to recompile includer
                    mNeedRecompileShaderFiles.insert(includer);
                    continue;
                }
                else
                {
                    // includer compilation failed, but we haven't updated includee since last compilation fail
                    // => skip for now
                    continue;
                }
            }

            // if there was no recompilation fail, simply check if last modification is newer then includer
            if(fs::last_write_time(includee) > fs::last_write_time(GetFileOutputPath(includer)))
            {
                // if yes, its time to recompile includer
                mNeedRecompileShaderFiles.insert(includer);
                continue;
            }
        }
    }

    void ShaderManager::CheckIncluderForRecompilation(const std::string& includer)
    {

        // check if includer recompilation failed
        if(Cache.FailedCompileTimestamps.find(includer) != Cache.FailedCompileTimestamps.end())
        {
            // check if indcluder has been modified since last crash
            if(fs::last_write_time(includer) > Cache.FailedCompileTimestamps[includer])
            {
                // if yes, its time to recompile includer
                mNeedRecompileShaderFiles.insert(includer);
                return;
            }
            else
            {
                // includer compilation failed, but we haven't updated includer since last compilation fail
                // => skip for now
                return;
            }
        }

        // if includer output binary is missing OR the source is newer then the output, trigger recompilation
        if(!fs::exists(includer) || fs::last_write_time(includer) > fs::last_write_time(GetFileOutputPath(includer)))
        {
            mNeedRecompileShaderFiles.insert(includer);
        }
    }
}  // namespace hsk