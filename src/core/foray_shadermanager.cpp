#include "foray_shadermanager.hpp"
#include "../foray_exception.hpp"
#include "../foray_logger.hpp"
#include "../util/foray_hash.hpp"
#include "foray_shadermodule.hpp"
#include <codecvt>
#include <filesystem>
#include <fstream>

namespace foray::core {

    namespace fs = std::filesystem;

#pragma region Hashing, File Time

    uint64_t ShaderManager::MakeHash(std::string_view absoluteUniqueSourceFilePath, const ShaderCompilerConfig& options)
    {
        // Use sets so the order as hashed is consistent
        std::set<std::string_view> includeDirs;
        includeDirs.insert(options.IncludeDirs.cbegin(), options.IncludeDirs.cend());
        std::set<std::string_view> definitions;
        definitions.insert(options.Definitions.cbegin(), options.Definitions.cend());
        std::set<std::string_view> additionalOptions;
        additionalOptions.insert(options.AdditionalOptions.cbegin(), options.AdditionalOptions.cend());

        size_t hash = {};
        util::AccumulateHash(hash, absoluteUniqueSourceFilePath);
        util::AccumulateHash(hash, includeDirs.size());
        for(std::string_view path : includeDirs)
        {
            util::AccumulateHash(hash, path);
        }
        util::AccumulateHash(hash, definitions.size());
        for(std::string_view def : definitions)
        {
            util::AccumulateHash(hash, def);
        }
        util::AccumulateHash(hash, options.EntryPoint);
        util::AccumulateHash(hash, additionalOptions.size());
        for(std::string_view option : additionalOptions)
        {
            util::AccumulateHash(hash, option);
        }
        return (uint64_t)hash;
    }

    fs::file_time_type ShaderManager::GetWriteTime(const osi::Utf8Path& path, WriteTimeLookup& writeTimeLookup)
    {
        auto iter = writeTimeLookup.find(path);
        if(iter != writeTimeLookup.end())
        {
            return iter->second;
        }
        fs::path           fspath    = path;
        bool               exists    = fs::status(fspath).type() == fs::file_type::regular;
        fs::file_time_type writeTime = fs::file_time_type::min();
        if(exists)
        {
            writeTime = fs::last_write_time(fspath);
        }
        writeTimeLookup[path] = writeTime;
        return writeTime;
    }

#pragma endregion
#pragma region Compile Shader

    uint64_t ShaderManager::CompileShader(osi::Utf8Path sourceFilePath, ShaderModule* shaderModule, const ShaderCompilerConfig& compileOptions, core::Context* context)
    {
        return CompileShader(sourceFilePath, *shaderModule, compileOptions, context);
    }
    uint64_t ShaderManager::CompileShader(osi::Utf8Path sourceFilePath, ShaderModule& shaderModule, const ShaderCompilerConfig& compileOptions, core::Context* context)
    {
        context = !context ? mContext : context;
        if(sourceFilePath.IsRelative())
        {
            sourceFilePath = sourceFilePath.MakeAbsolute();
        }


        bool exists = fs::exists((fs::path)sourceFilePath);
        bool isFile = !fs::is_directory((fs::path)sourceFilePath);
        FORAY_ASSERTFMT(exists && isFile, "[ShaderManager::GetShaderBinary] Shader source file \"{}\" does not exist or is not a file!", (const std::string&)sourceFilePath);

        uint64_t compileHash = MakeHash((const std::string&)sourceFilePath, compileOptions);

        auto iter = mTrackedCompilations.find(compileHash);
        if(iter != mTrackedCompilations.end())
        {
            const osi::Utf8Path& spvPath = iter->second->SpvPath;
            shaderModule.LoadFromFile(context, spvPath);
            return iter->second->Hash;
        }

        std::unique_ptr<ShaderCompilation>& compilation = mTrackedCompilations[compileHash] =
            std::make_unique<ShaderCompilation>(this, sourceFilePath, compileOptions, compileHash);

        compilation->FindIncludes();
        WriteTimeLookup     lookup;
        ECompileCheckResult checkResult = compilation->NeedsCompile(lookup);
        if(checkResult == ECompileCheckResult::NeedsRecompile)
        {
            compilation->Compile();
        }
        Assert(checkResult != ECompileCheckResult::MissingInput, "Missing Input file");

        const osi::Utf8Path& spvPath = compilation->SpvPath;
        shaderModule.LoadFromFile(context, spvPath);
        return compileHash;
    }

#pragma endregion
#pragma region Call GLSL Compiler

#ifdef FORAY_DEBUG
#define OPTIMIZE "-O0"
#else
#define OPTIMIZE "-O"
#endif

#ifdef _WIN32

    // @brief calls the glslc.exe on windows and passes the shader file path
    // returns false if the compilation failed
    bool ShaderManager::CallGlslCompiler(std::string_view args)
    {
        // convert paths to wstring
        std::wstring argsW = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string(args));
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

        std::wstring commandLine = std::wstring(lpApplicationName) + L" --target-spv=spv1.5" + argsW;
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
    bool ShaderManager::CallGlslCompiler(std::string_view args)
    {
        std::string command     = fmt::format("/bin/glslc --target-spv=spv1.5{} {}", args, OPTIMIZE);
        int         returnvalue = std::system(command.c_str());
        return returnvalue == 0;
    }
#endif

#pragma endregion
#pragma region ShaderCompilation

    ShaderManager::ShaderCompilation::ShaderCompilation(ShaderManager* manager, const osi::Utf8Path& source, const ShaderCompilerConfig& options, uint64_t hash)
        : Manager(manager), SourcePath(source), Config(options), Hash(hash)
    {
        SpvPath = fmt::format("{}.{:x}.spv", source, hash);
        WriteTimeLookup lookup;
        LastCompile = GetWriteTime(SpvPath, lookup);
    }

    bool ShaderManager::ShaderCompilation::Compile()
    {
        logger()->info("Beginning compilation of \033[1m{}\033[m (\033[\033[2m0x{:x}\033[m)", SourcePath, Hash);
        std::string args;
        {
            std::stringstream strbuilder;
            for(const osi::Utf8Path& includeDir : Config.IncludeDirs)
            {
                strbuilder << " -I \"" << includeDir << "\"";
            }
            for(const std::string& definition : Config.Definitions)
            {
                strbuilder << " -D" << definition;
            }
            for(const std::string& option : Config.AdditionalOptions)
            {
                strbuilder << " " << option;
            }
            strbuilder << " -o \"" << SpvPath << "\"";
            strbuilder << " \"" << SourcePath << "\"";

            args = strbuilder.str();
        }
        if(Manager->CallGlslCompiler(args))
        {
            // Check output file
            fs::path        path   = SpvPath;
            fs::file_status status = fs::status(path);
            bool            exists = status.type() == fs::file_type::regular;
            if(exists)
            {
                logger()->info("\033[32mSUCCESS\033[m compiling \033[1m{}\033[m (\033[\033[2m0x{:x}\033[m)", SourcePath, Hash);
                LastCompile = fs::last_write_time(path);
                return true;
            }
        }
        logger()->info("\033[31mFAILURE\033[m compiling \033[1m{}\033[m (\033[\033[2m0x{:x}\033[m).", SourcePath, Hash);
        LastFailedCompile = fs::file_time_type::clock::now();
        return false;
    }

    ShaderManager::ECompileCheckResult ShaderManager::ShaderCompilation::NeedsCompile(std::unordered_map<osi::Utf8Path, std::filesystem::file_time_type>& writeTimeLookup)
    {
        // Update last compile
        fs::file_time_type lastCompile = std::max({LastCompile, LastFailedCompile, GetWriteTime(SpvPath, writeTimeLookup)});

        bool outOfDate = false;

        {  // Check shader file
            fs::file_time_type lastWriteTime = GetWriteTime(SourcePath, writeTimeLookup);
            if(lastWriteTime == fs::file_time_type::min())
            {
                return ECompileCheckResult::MissingInput;
            }
            outOfDate = outOfDate || lastWriteTime > lastCompile;
        }

        // Check all includes to make sure they still exist and haven't been updated
        for(IncludeFile* include : Includes)
        {
            fs::file_time_type lastWriteTime = GetWriteTime(include->Path, writeTimeLookup);
            if(lastWriteTime == fs::file_time_type::min())
            {
                return ECompileCheckResult::MissingInput;
            }
            outOfDate = outOfDate || lastWriteTime > lastCompile;
        }
        return outOfDate ? ECompileCheckResult::NeedsRecompile : ECompileCheckResult::UpToDate;
    }

#pragma endregion
#pragma region IncludeFile

    void ShaderManager::IncludeFile::GetIncludesRecursively(std::unordered_set<IncludeFile*>& out)
    {
        for(IncludeFile* include : Includees)
        {
            if(!out.contains(include))
            {
                out.emplace(include);
                include->GetIncludesRecursively(out);
            }
        }
    }

#pragma endregion
#pragma region Include File Search

    void findIncludeDirectives(const osi::Utf8Path& path, std::vector<osi::Utf8Path>& outIncludeDirectives)
    {
        std::ifstream     ifstream((fs::path)path);
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
            outIncludeDirectives.push_back(content.substr(firstOccurance + 1, secondOccurance - firstOccurance - 1));
            currentPos = secondOccurance + 1;
        }
    }

    ShaderManager::IncludeFile* ShaderManager::RecursivelyProcessInclude(const osi::Utf8Path& path, const std::vector<osi::Utf8Path>& extraIncludeDirs)
    {
        auto iter = mTrackedIncludeFiles.find(path);
        if(iter != mTrackedIncludeFiles.end())
        {
            return iter->second.get();
        }

        IncludeFile* include = (mTrackedIncludeFiles[path] = std::make_unique<IncludeFile>(path)).get();

        std::vector<osi::Utf8Path> includeDirectives;

        findIncludeDirectives(path, includeDirectives);

        std::vector<osi::Utf8Path> includePaths;
        includePaths.reserve(extraIncludeDirs.size() + 1);
        includePaths.push_back(osi::ToUtf8Path(((fs::path)path).remove_filename()));
        includePaths.insert(includePaths.cend(), extraIncludeDirs.cbegin(), extraIncludeDirs.cend());

        for(const osi::Utf8Path& includeDirective : includeDirectives)
        {
            bool foundInclude = false;
            for(const osi::Utf8Path& dir : includePaths)
            {
                osi::Utf8Path absIncludePath;
                try
                {
                    absIncludePath = dir / includeDirective;
                }
                catch(const std::exception& e)
                {
                    continue;  // There might be includePaths which can yield illegal paths (navigating above root directory)
                }
                bool exists = fs::status(absIncludePath).type() == fs::file_type::regular;
                if(exists)
                {
                    IncludeFile* child = RecursivelyProcessInclude(absIncludePath, extraIncludeDirs);
                    include->Includees.emplace(child);
                    break;
                }
            }
        }

        return include;
    }

    void ShaderManager::ShaderCompilation::FindIncludes()
    {
        // Find include directives
        std::vector<osi::Utf8Path> includeDirectives;

        std::vector<osi::Utf8Path> includePaths;
        includePaths.reserve(Config.IncludeDirs.size() + 1);
        includePaths.push_back(osi::ToUtf8Path(((fs::path)SourcePath).remove_filename()));
        includePaths.insert(includePaths.cend(), Config.IncludeDirs.cbegin(), Config.IncludeDirs.cend());

        findIncludeDirectives(SourcePath, includeDirectives);

        // Process include files

        for(const osi::Utf8Path& includeDirective : includeDirectives)
        {
            bool foundInclude = false;
            for(const osi::Utf8Path& dir : includePaths)
            {
                osi::Utf8Path absIncludePath;
                try
                {
                    absIncludePath = dir / includeDirective;
                }
                catch(const std::exception& e)
                {
                    continue;  // There might be includePaths which can yield illegal paths (navigating above root directory)
                }
                bool exists = fs::status(absIncludePath).type() == fs::file_type::regular;
                if(exists)
                {
                    IncludeFile* child = Manager->RecursivelyProcessInclude(absIncludePath, Config.IncludeDirs);
                    Includes.emplace(child);
                    child->GetIncludesRecursively(Includes);
                    break;
                }
            }
        }

        // Register shadercompilation at include files

        for(IncludeFile* include : Includes)
        {
            include->Includers.emplace(this);
        }
    }

#pragma endregion
#pragma region Check and Update

    bool ShaderManager::CheckAndUpdateShaders(std::unordered_set<uint64_t>& out_recompiled)
    {
        bool result = false;

        WriteTimeLookup lookup;
        for(auto& entry : mTrackedCompilations)
        {
            ShaderCompilation*  compilation = entry.second.get();
            ECompileCheckResult check       = compilation->NeedsCompile(lookup);
            if(check == ECompileCheckResult::NeedsRecompile)
            {
                if(compilation->Compile())
                {
                    out_recompiled.emplace(compilation->Hash);
                    result = true;
                }
            }
        }

        return result;
    }

#pragma endregion

}  // namespace foray::core