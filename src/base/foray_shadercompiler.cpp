#include "foray_shadercompiler.hpp"
#include "../core/foray_logger.hpp"
#include "../foray_exception.hpp"
#include "../osi/foray_env.hpp"
#include <codecvt>

namespace foray::base {

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
    // https://stackoverflow.com/questions/15435994/how-do-i-open-an-exe-from-another-c-exe
    inline SPV_STR PathToString(const fs::path& path)
    {
        return path.wstring();
    }
#endif

    void ShaderCompiler::AddSourceDirectory(const std::string& sourceDirectory)
    {
#ifdef _WIN32
        //mSourceDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(sourceDir);
        mSourceDirectories.push_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(sourceDirectory));
#else
        mSourceDirectories.push_back(sourceDirectory);
#endif  // _WIN32
    }

    void ShaderCompiler::SetOutputDirectory(const std::string& outputDirectory)
    {
#ifdef _WIN32
        //mSourceDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(sourceDir);
        mOutputDir = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(outputDirectory);
#else
        mOutputDir = outputDirectory;
#endif  // _WIN32
    }

#ifdef _WIN32
    // calls the glslc.exe on windows and passes the shader file path
    // returns false if the compilation failed
    bool ShaderCompiler::CallGlslCompiler(const ShaderFileInfo& shaderFileInfo)
    {
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

        SPV_STR commandLine =
            SPV_STR(lpApplicationName) + L" --target-spv=spv1.5 " + PathToString(shaderFileInfo.mSourcePathFull) + L" -o " + PathToString(shaderFileInfo.mOutPathFull);
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

#define SPIRV_FILEENDING std::wstring(L".spv")
#else
    inline SPV_STR PathToString(const fs::path& path)
    {
        return path.string();
    }

    // returns false if compilation fails
    bool ShaderCompiler::CallGlslCompiler(const ShaderFileInfo& shaderFileInfo)
    {
        std::string command("/bin/glslc --target-spv=spv1.5 " + PathToString(shaderFileInfo.mSourcePathFull) + " -o " + PathToString(shaderFileInfo.mOutPathFull));
        int returnvalue = std::system(command.c_str());
        return returnvalue == 0;
    }

#define SPIRV_FILEENDING std::string(".spv")
#endif


    bool ShaderCompiler::CompileAll(bool recompile)
    {
        if(mSourceDirectories.size() == 0)
        {
            throw Exception("Tried to compile all shaders, but no source directory to read files from was specified!");
        }
        // parse all shaders in data directory
        for(auto& sourceDirectory : mSourceDirectories)
        {
            if(!fs::exists(sourceDirectory))
            {
                if((mVerbosityFlags & VerbosityFlags::Verbose) == VerbosityFlags::Verbose)
                {
                    core::logger()->warn("ShaderCompiler: Skipping source directory \"{}\" because it does not exist.", osi::ToUtf8Path(sourceDirectory));
                }
                continue;
            }
            for(auto& pathIterator : fs::recursive_directory_iterator(sourceDirectory))
            {
                if(!pathIterator.is_directory())
                {
                    fs::path shaderFile = pathIterator.path();
                    CompileShaderFile(sourceDirectory, shaderFile, recompile);
                }
            }
        }
        return true;
    }

    bool ShaderCompiler::CompileShaderFile(fs::path sourceDir, fs::path shaderFilePath, bool recompile)
    {
        if(!IsValidSourceFile(shaderFilePath))
        {
            if((mVerbosityFlags & VerbosityFlags::Verbose) == VerbosityFlags::Verbose)
            {
                core::logger()->info("unsupported file extension: {}", shaderFilePath.string());
            }
            return false;
        }

        SPV_STR outputFullPath;
        if(mOutputDir.length() > 0)
        {
            // shader output directory is set
            outputFullPath = PathToString(mOutputDir) + PathToString(fs::relative(shaderFilePath, sourceDir)) + SPIRV_FILEENDING;
        }
        else
        {
            // shader output directory not explictly set .. put .spv next to sources
            outputFullPath = PathToString(shaderFilePath) + SPIRV_FILEENDING;
        }
        ShaderFileInfo shaderFileInfo{shaderFilePath, fs::path(outputFullPath)};


        LogLastModified(shaderFileInfo);


        if(!NeedsCompiling(shaderFileInfo))
        {
            return true;
        }

        bool compileResult = CallGlslCompiler(shaderFileInfo);

        if(!compileResult)
        {
            core::logger()->error("Failed to compile: {} ", shaderFileInfo.mSourcePathFull.string());
            if(mThrowException)
            {
                throw foray::Exception("Shader compile failed for file: {}", shaderFileInfo.mSourcePathFull.string());
            }
        }

        if(recompile)
        {
            mMapShadersRecompiled[shaderFileInfo.mOutPathFull.filename().string()] = true;
        }

        // If you get a runtime exception here, check console for shader compile error messages
        return compileResult;
    }

    bool ShaderCompiler::HasShaderBeenRecompiled(std::string& shaderFilePath, bool invalidate)
    {
        std::string shaderFileName = std::filesystem::path(shaderFilePath).filename().string();
        bool        recompiled     = false;
        if(mMapShadersRecompiled.find(shaderFileName) != mMapShadersRecompiled.end())
        {
            // true if shaders been recompiled since last check
            recompiled = mMapShadersRecompiled[shaderFileName];
        }
        if(invalidate)
        {
            // next call returns false, until another recompilation happens
            mMapShadersRecompiled[shaderFileName] = false;
        }
        return recompiled;
    }

    bool ShaderCompiler::IsValidSourceFile(const fs::path& shaderFilePath)
    {
        SPV_STR pathName = PathToString(shaderFilePath);
        for(auto& fileEnding : mValidFileEndings)
        {
            if(EndsWith(pathName, fileEnding))
                return true;
        }

        // cannot identify shader type by one of the preset file endings
        return false;
    }

    bool ShaderCompiler::NeedsCompiling(const ShaderFileInfo& shaderFile)
    {
        if(!fs::exists(shaderFile.mOutPathFull))
        {
            return true;
        }

        fs::file_time_type ftimeSource    = fs::last_write_time(shaderFile.mSourcePathFull);
        fs::file_time_type ftimeOutput    = fs::last_write_time(shaderFile.mOutPathFull);
        auto               milliseconds   = std::chrono::duration_cast<std::chrono::milliseconds>(ftimeOutput - ftimeSource);
        bool               needsCompiling = milliseconds.count() < 0;

        return needsCompiling;
    }

    void ShaderCompiler::LogLastModified(const ShaderFileInfo& shaderFileInfo)
    {

        fs::file_time_type ftimeSource = fs::last_write_time(shaderFileInfo.mSourcePathFull);
        if(!fs::exists(shaderFileInfo.mOutPathFull))
        {
            if((mVerbosityFlags & VerbosityFlags::Modified) == VerbosityFlags::Modified)
            {
                // if file does not exist, compile it.
                core::logger()->info("last compiled \033[1;31mnever\033[0m COMPILING NOW - last modified \033[1;32m{}\033[0m - {} ({})", FileTimeToString(ftimeSource),
                               shaderFileInfo.mSourcePathFull.filename().string(), shaderFileInfo.mSourcePathFull.string());
            }
            return;
        }

        fs::file_time_type ftimeOutput    = fs::last_write_time(shaderFileInfo.mOutPathFull);
        auto               milliseconds   = std::chrono::duration_cast<std::chrono::milliseconds>(ftimeOutput - ftimeSource);
        bool               needsCompiling = milliseconds.count() < 0;
        // colors https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
        if(needsCompiling)
        {
            if((mVerbosityFlags & VerbosityFlags::Modified) == VerbosityFlags::Modified)
            {
                core::logger()->info("last compiled \033[1;31m{}\033[0m - last modified \033[1;32m{}\033[0m - {} ({})", FileTimeToString(ftimeOutput),
                                     FileTimeToString(ftimeSource), shaderFileInfo.mSourcePathFull.filename().string(), shaderFileInfo.mSourcePathFull.string());
                core::logger()->info("==> recompiling {} ...", shaderFileInfo.mSourcePathFull.filename().string());
            }
        }
        else
        {
            if((mVerbosityFlags & VerbosityFlags::Unmodified) == VerbosityFlags::Unmodified)
            {
                core::logger()->info("last compiled \033[1;32m{}\033[0m - last modified \033[1;32m{}\033[0m - {} ({})", FileTimeToString(ftimeOutput),
                                     FileTimeToString(ftimeSource), shaderFileInfo.mSourcePathFull.filename().string(), shaderFileInfo.mSourcePathFull.string());
            }
        }
    }

    // https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c
    bool ShaderCompiler::EndsWith(const SPV_STR& str, const SPV_STR& suffix)
    {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }

    bool ShaderCompiler::StartsWith(const SPV_STR& str, const SPV_STR& prefix)
    {
        return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
    }

    template <typename TP>
    std::time_t to_time_t(TP tp)
    {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
        return system_clock::to_time_t(sctp);
    }

    std::string ShaderCompiler::FileTimeToString(fs::file_time_type filetime)
    {
        std::time_t       tt  = to_time_t(filetime);
        std::tm*          gmt = std::gmtime(&tt);
        std::stringstream buffer;
        // https://en.cppreference.com/w/cpp/io/manip/put_time
        buffer << std::put_time(gmt, "%d/%b/%y %H:%M");
        std::string formattedFileTime = buffer.str();
        return formattedFileTime;
    }
}  // namespace foray::base