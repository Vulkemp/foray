#include "hsk_shadermanager.hpp"
#include "../hsk_env.hpp"
#include <regex>

namespace hsk {

    const std::vector<char>& ShaderManager::GetShaderBinary(std::string relativeFilePath)
    {
        auto absolutePath = MakeRelativePath(relativeFilePath);
        return GetShaderBinaryAbsolutePath(absolutePath);
    }

    const std::vector<char>& ShaderManager::GetShaderBinaryAbsolutePath(std::string shaderSourceFilePath)
    {
        // clean shader file path
        NormalizePath(shaderSourceFilePath);
        Cache.CurrentMainIncluder = shaderSourceFilePath;

        std::string outputFilePath = GetFileOutputPath(shaderSourceFilePath);

        if(mSpirvCodeMap.find(outputFilePath) == mSpirvCodeMap.end())
        {
            // output file was not yet listed in spirvmap
            // => do initial scan which tracks all included files
            RecursiveScanIncludes(shaderSourceFilePath);
        }

        // if either outputfile not exists,
        // outputfile outdated
        // or any include file outdated
        // => recompile
        if(!fs::exists(outputFilePath) || fs::last_write_time(shaderSourceFilePath) > fs::last_write_time(outputFilePath) || RecursiveScanIncludes(shaderSourceFilePath))
        {
            CallGlslCompiler(shaderSourceFilePath, outputFilePath);
            AddSpirvFile(outputFilePath);
        }
        
        return (*mSpirvCodeMap.find(outputFilePath)).second;
    }

#ifdef _WIN32
    // calls the glslc.exe on windows and passes the shader file path
    // returns false if the compilation failed
    bool ShaderManager::CallGlslCompiler(const std::string& inputFilePath, const std::string& outputFilePath)
    {
        // convert paths to wstring
        std::wstring inputFilePathW  = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(inputFilePath);
        std::wstring outputFilePathW = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(outputFilePath);
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
    bool ShaderManager::CallGlslCompiler(const std::string& inputFilePath, const std::string& outputFilePath)
    {
        std::string command("/bin/glslc --target-spv=spv1.5 " + inputFilePath + " -o " + outputFilePath);
        int         returnvalue = std::system(command.c_str());
        return returnvalue == 0;
    }
#endif

    bool ShaderManager::HasShaderBeenRecompiledRelativePath(std::string& inputFilePath) {
        auto absolutePath = MakeRelativePath(inputFilePath);
        NormalizePath(absolutePath);
        return HasShaderBeenRecompiledAbsolutePath(absolutePath);
    }

    bool ShaderManager::RecursiveScanIncludes(std::string& inputFilePath, uint32_t recursionDepth)
    {
        if(recursionDepth > mMaxRecursionDepth)
        {
            throw Exception("ShaderManager max recursion depth reached. Your shader files might have circular includes.");
        }

        if(Cache.TrackedFilesLookup.find(inputFilePath) == Cache.TrackedFilesLookup.end())
        {
            Cache.TrackedFilesLookup.insert(inputFilePath);
            Cache.TrackedFilesIterate.push_back(inputFilePath);
        }

        std::vector<std::string> includes;
        GetAllIncludes(inputFilePath, &includes);

        bool anyIncludeChanged = false;
        for(std::string& includee : includes)
        {
            // get includee path
            std::string includeeFullPath = GetIncludeFilePathRelativeToFile(inputFilePath, includee);

            // add includee's includer
            Cache.AddMainFileToIncludee(includeeFullPath);

            // check all includes of includee
            anyIncludeChanged = RecursiveScanIncludes(includeeFullPath, ++recursionDepth) || anyIncludeChanged;
        }

        std::string outputFilePath = GetFileOutputPath(inputFilePath);

        // check if includee was modified
        if(!fs::exists(outputFilePath) || fs::last_write_time(inputFilePath) > fs::last_write_time(outputFilePath))
        {
            anyIncludeChanged = true;
        }
        return anyIncludeChanged;
    }

    void ShaderManager::SplitString(std::string const& str, const char delim, std::vector<std::string>& out)
    {
        size_t start;
        size_t end = 0;

        while((start = str.find_first_not_of(delim, end)) != std::string::npos)
        {
            end = str.find(delim, start);
            out.push_back(str.substr(start, end - start));
        }
    }

    std::string ShaderManager::GetIncludeFilePathRelativeToFile(std::string& includer, const std::string& includee)
    {
        // remove file from includer path and append include
        std::filesystem::path includerPath(includer);
        includerPath.remove_filename();
        includerPath.append(includee);

        std::string newPath = includerPath.string();
        NormalizePath(newPath);
        return newPath;
    }

    void ShaderManager::NormalizePath(std::string& inputFilePath)
    {
        // replace all \\ to /
        inputFilePath = std::regex_replace(inputFilePath, std::regex("\\\\"), "/");

        // clean path from all ../
        std::vector<std::string> outStringParts;
        SplitString(inputFilePath, '/', outStringParts);
        for(size_t i = outStringParts.size() - 1; i > 0; i--)
        {
            if(outStringParts[i] == "..")
            {
                outStringParts[i] = "";
                size_t j          = i - 1;
                while(j >= 0 && (outStringParts[j] == ".." || outStringParts[j].length() == 0))
                    j--;
                outStringParts[j] = "";
            }
        }

        // rebuild string from tokens
        inputFilePath = "";
        for(size_t i = 0; i < outStringParts.size(); i++)
        {
            inputFilePath.append(outStringParts[i]);
            if(i < outStringParts.size() - 1 && outStringParts[i].length() > 0)
                inputFilePath.append("/");
        }
    }

    bool ShaderManager::CheckTrackedFilesForModification() { 
        mNeedRecompileShaderFiles.clear();
        bool anyModification = false;
        for(std::string& inputFilePath : Cache.TrackedFilesIterate)
        {
            auto mapIncludeeToIncluders = Cache.Includees.find(inputFilePath);
            // is file and includee
            if(mapIncludeeToIncluders != Cache.Includees.end())
            {
                for(const std::string& includer : (*mapIncludeeToIncluders).second)
                {
                    std::string includerOutputPath = GetFileOutputPath(includer);
                    // check if includee is newer then includer
                    if(fs::last_write_time(inputFilePath) > fs::last_write_time(includerOutputPath))
                    {
                        // add includer to recompile list.
                        mNeedRecompileShaderFiles.insert(includer);
                        anyModification = true;
                    }
                }
            }
            else // if file is not includee, must be includer
            {
                
                std::string includerOutputPath = GetFileOutputPath(inputFilePath);
                // check if includer source is newer then spirv binary
                if(fs::last_write_time(inputFilePath) > fs::last_write_time(includerOutputPath))
                {
                    mNeedRecompileShaderFiles.insert(inputFilePath);
                    anyModification = true;
                }
            }
            
        }
        return anyModification;
    }

    void ShaderManager::RecompileModifiedShaders() {
        for(std::string toRecompilePath : mNeedRecompileShaderFiles)
        {
            CallGlslCompiler(toRecompilePath, GetFileOutputPath(toRecompilePath));
        }
    }

    void ShaderManager::GetAllIncludes(std::string& inputFilePath, std::vector<std::string>* includes)
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

    void ShaderManager::AddSpirvFile(std::string& shaderSpirvFullPath)
    {
        std::ifstream file(shaderSpirvFullPath.c_str(), std::ios::binary | std::ios::in | std::ios::ate);

        if(!file.is_open())
        {
            throw Exception("Could not open shader file: {}", shaderSpirvFullPath);
        }

        // create buffer if not existent
        if(mSpirvCodeMap.find(shaderSpirvFullPath) == mSpirvCodeMap.end())
        {
            mSpirvCodeMap.insert({shaderSpirvFullPath, std::vector<char>()});
        }
        // get buffer reference
        auto buffer = &(*mSpirvCodeMap.find(shaderSpirvFullPath)).second;

        size_t fileSize = (size_t)file.tellg();
        buffer->resize(fileSize);
        file.seekg(0);
        file.read(buffer->data(), static_cast<std::streamsize>(fileSize));
        file.close();

    }

}  // namespace hsk