#include <Core/Containers/ByteBuffer.h>
#include <Core/IO/Async.h>
#include <Core/IO/FileStream.h>
#include <Core/Logging/Trace.h>
#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/PrecompiledShaders.h>
#include <Graphics/Core/ShaderCompilerDXC.h>
#include <Graphics/Core/ShaderPermutationManifest.h>

#define near
#define far
#include <d3d12shader.h>
#undef near
#undef far

namespace FE::Graphics::Core
{
    namespace
    {
        constexpr const char* kCompilerArgs[] = { "-spirv",
                                                  "-fspv-target-env=vulkan1.3",
                                                  "-fspv-extension=KHR",
                                                  "-fspv-extension=SPV_EXT_descriptor_indexing",
                                                  "-fspv-extension=SPV_EXT_mesh_shader",
                                                  "-fspv-extension=SPV_GOOGLE_hlsl_functionality1",
                                                  "-fspv-extension=SPV_GOOGLE_user_type",
                                                  "-fspv-preserve-interface",
                                                  "-fspv-preserve-bindings",
                                                  "-fspv-reflect",
                                                  "-fvk-use-dx-layout",
                                                  "-fvk-use-dx-position-w",
                                                  "-Zpr" };

        constexpr const char* kDebugCompilerArgs[] = { "-Od", "-Zi" };
        constexpr const char* kOptimizedCompilerArgs[] = { "-O3" };

        //! Bump when compiled shaders change without a change of the compiler or its arguments.
        constexpr uint64_t kShaderCacheVersion = 1;


        LPWSTR ConvertString(std::pmr::memory_resource* allocator, const festd::string_view source)
        {
            const Str::Utf8ToUtf16 convert{ source.data(), source.size() };

            const auto buffer = static_cast<WCHAR*>(allocator->allocate((convert.size() + 1) * sizeof(WCHAR)));
            memcpy(buffer, convert.data(), convert.size() * sizeof(WCHAR));
            buffer[convert.size()] = '\0';

            return buffer;
        }


        LPCWSTR GetShaderTargetProfile(const ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::kVertex:
                return L"vs_6_7";
            case ShaderStage::kAmplification:
                return L"as_6_7";
            case ShaderStage::kMesh:
                return L"ms_6_7";
            case ShaderStage::kPixel:
                return L"ps_6_7";
            case ShaderStage::kCompute:
                return L"cs_6_7";
            case ShaderStage::kUndefined:
            default:
                FE_Assert(false, "Invalid ShaderStage");
                return nullptr;
            }
        }


        //! Resolves includes through the source cache and records every file used by one compilation.
        struct DxcIncludeHandler final : public IDxcIncludeHandler
        {
            explicit DxcIncludeHandler(IDxcUtils* dxcUtils, ShaderSourceCache* shaderSourceCache)
                : m_dxcUtils(dxcUtils)
                , m_shaderSourceCache(shaderSourceCache)
            {
            }

            void AddDependency(const ShaderSourceFile* sourceFile)
            {
                for (const ShaderCacheDependency& dependency : m_dependencies)
                {
                    if (dependency.m_path == sourceFile->GetPath())
                        return;
                }

                ShaderCacheDependency& dependency = m_dependencies.emplace_back();
                dependency.m_path = sourceFile->GetPath();
                dependency.m_contentHash = sourceFile->GetContentHash();
            }

            [[nodiscard]] festd::span<const ShaderCacheDependency> GetDependencies() const
            {
                return festd::span{ m_dependencies.data(), m_dependencies.size() };
            }

            HRESULT STDMETHODCALLTYPE LoadSource(const LPCWSTR filenameWide, IDxcBlob** includeSource) override
            {
                FE_PROFILER_ZONE();

                const Str::Utf16ToUtf8 pathUtf8{ filenameWide };
                const IO::Path path = IO::NormalizePath({ pathUtf8.data(), pathUtf8.size() });

                const Env::Name name{ path };
                if (const festd::expected result = m_shaderSourceCache->GetSource(name, ShaderSourceReadMode::kBlocking))
                {
                    const Rc<ShaderSourceFile>& sourceFile = result.value();
                    const festd::string_view source = sourceFile->GetSource();

                    Rc<IDxcBlobEncoding> blobEncoding;
                    const HRESULT hr =
                        m_dxcUtils->CreateBlobFromPinned(source.data(), source.size(), DXC_CP_UTF8, blobEncoding.GetAddressOf());
                    if (SUCCEEDED(hr))
                    {
                        AddDependency(sourceFile.Get());
                        *includeSource = blobEncoding.Detach();
                        return S_OK;
                    }

                    return hr;
                }

                return E_FAIL;
            }

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override
            {
                return E_NOTIMPL;
            }

            ULONG STDMETHODCALLTYPE AddRef() override
            {
                return ++m_refCount;
            }

            ULONG STDMETHODCALLTYPE Release() override
            {
                const uint32_t refCount = --m_refCount;
                if (refCount == 0)
                    Memory::DefaultDelete(this);

                return refCount;
            }

        private:
            IDxcUtils* m_dxcUtils = nullptr;
            ShaderSourceCache* m_shaderSourceCache = nullptr;
            festd::inline_vector<ShaderCacheDependency, 16> m_dependencies;
            std::atomic<ULONG> m_refCount = 0;
        };


        void AppendDependencies(ShaderCompilerResult& result, const festd::span<const ShaderCacheDependency> dependencies)
        {
            for (const ShaderCacheDependency& dependency : dependencies)
            {
                result.m_dependencies.push_back(dependency.m_path);
                result.m_dependencyHashes.push_back(dependency.m_contentHash);
            }
        }


        ShaderCompilerResult MakeFailedResult(const festd::span<const ShaderCacheDependency> dependencies)
        {
            ShaderCompilerResult result;
            AppendDependencies(result, dependencies);
            return result;
        }


        uint64_t GetCompilerVersionHash(IDxcCompiler3* compiler)
        {
            Hasher hasher;

            Rc<IDxcVersionInfo> versionInfo;
            if (SUCCEEDED(compiler->QueryInterface(IID_PPV_ARGS(versionInfo.GetAddressOf()))))
            {
                UINT32 major = 0;
                UINT32 minor = 0;
                if (SUCCEEDED(versionInfo->GetVersion(&major, &minor)))
                    hasher.UpdateRaw(major).UpdateRaw(minor);
            }

            Rc<IDxcVersionInfo2> commitInfo;
            if (SUCCEEDED(compiler->QueryInterface(IID_PPV_ARGS(commitInfo.GetAddressOf()))))
            {
                UINT32 commitCount = 0;
                char* commitHash = nullptr;
                if (SUCCEEDED(commitInfo->GetCommitInfo(&commitCount, &commitHash)))
                {
                    hasher.UpdateRaw(commitCount);
                    if (commitHash != nullptr)
                    {
                        hasher.Update(commitHash, strlen(commitHash));
                        CoTaskMemFree(commitHash);
                    }
                }
            }

            return hasher.Finalize();
        }
    } // namespace


    ShaderCompilerConfig ResolveShaderCompilerConfig(const ShaderCompilerConfig config)
    {
        if (config != ShaderCompilerConfig::kDefault)
            return config;

        return Build::IsDebug() ? ShaderCompilerConfig::kDebug : ShaderCompilerConfig::kOptimized;
    }


    uint64_t ComputeShaderRequestKey(const ShaderCompilerArgs& args)
    {
        // Bump when the meaning of a request changes, e.g. when the compiler arguments of a configuration change.
        constexpr uint64_t kRequestKeyVersion = 1;

        Hasher hasher;
        hasher.UpdateRaw(kRequestKeyVersion)
            .UpdateRaw(args.m_shaderName.GetHash())
            .UpdateRaw(festd::to_underlying(args.m_stage))
            .UpdateRaw(festd::to_underlying(ResolveShaderCompilerConfig(args.m_config)));

        for (const ShaderDefine& define : args.m_defines)
            hasher.UpdateRaw(define.GetHash());

        return hasher.Finalize();
    }


    Rc<ShaderCompiler> CreateShaderCompiler()
    {
        return Memory::DefaultNew<ShaderCompilerDXC>();
    }


    ShaderCompilerDXC::ShaderCompilerDXC()
        : m_shaderCache(GetShaderCacheDirectory())
    {
        m_shaderSourceCache = Memory::DefaultNew<ShaderSourceCache>();
    }


    bool ShaderCompilerDXC::EnsureCompiler()
    {
        if (m_isCompilerLoaded.load(std::memory_order_acquire))
            return true;

        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_compilerLock };
        if (m_isCompilerLoaded.load(std::memory_order_relaxed))
            return true;

        if (m_hasCompilerFailedToLoad)
            return false;

        const auto dxcCreateInstance =
            m_module.Load("dxcompiler") ? m_module.FindFunction<DxcCreateInstanceProc>("DxcCreateInstance") : nullptr;
        const bool isCreated = dxcCreateInstance != nullptr
            && SUCCEEDED(dxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_dxcUtils.ReleaseAndGetAddressOf())))
            && SUCCEEDED(dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_dxcCompiler.ReleaseAndGetAddressOf())));
        if (!isCreated)
        {
            Logger::LogError("Failed to load dxcompiler.dll; only precompiled shaders are available");
            m_hasCompilerFailedToLoad = true;
            return false;
        }

        m_compilerVersionHash = GetCompilerVersionHash(m_dxcCompiler.Get());
        m_isCompilerLoaded.store(true, std::memory_order_release);
        return true;
    }


    bool ShaderCompilerDXC::TryLoadPrecompiled(const ShaderCompilerArgs& args, ShaderCompilerResult& result) const
    {
        FE_PROFILER_ZONE();

        const Rc precompiledShaders = GetPrecompiledShaders();
        if (!precompiledShaders)
            return false;

        festd::inline_vector<const PrecompiledShader*, 4> candidates;
        precompiledShaders->Find(ComputeShaderRequestKey(args), candidates);

        for (const PrecompiledShader* candidate : candidates)
        {
            const uint32_t alignedSize = AlignUp<sizeof(uint32_t)>(candidate->m_byteCodeSize);
            if (candidate->m_dependencies.empty() || candidate->m_byteCode.size() * sizeof(uint32_t) != alignedSize)
                continue;

            bool isCurrent = true;
            for (const PrecompiledShaderDependency& dependency : candidate->m_dependencies)
            {
                const festd::expected source = m_shaderSourceCache->GetSource(dependency.m_path);
                isCurrent &= source && source.value()->GetContentHash() == dependency.m_contentHash;
            }

            if (!isCurrent)
                continue;

            result.m_byteCode = ByteBuffer(alignedSize, args.m_binaryAllocator);
            memcpy(result.m_byteCode.data(), candidate->m_byteCode.data(), alignedSize);
            result.m_byteCodeSize = candidate->m_byteCodeSize;
            result.m_hash = DefaultHash(result.m_byteCode.data(), result.m_byteCodeSize);
            result.m_hashValid = true;
            result.m_codeValid = true;
            for (const PrecompiledShaderDependency& dependency : candidate->m_dependencies)
            {
                result.m_dependencies.push_back(dependency.m_path);
                result.m_dependencyHashes.push_back(dependency.m_contentHash);
            }

            return true;
        }

        return false;
    }


    ShaderCompilerResult ShaderCompilerDXC::CompileShader(const ShaderCompilerArgs& args)
    {
        FE_PROFILER_ZONE();

        RecordShaderPermutation(args);

        ShaderCompilerResult precompiledResult;
        if (TryLoadPrecompiled(args, precompiledResult))
            return precompiledResult;

        const festd::expected sourceResult = m_shaderSourceCache->GetSource(args.m_shaderName);
        if (!sourceResult)
        {
            const festd::string_view resultDesc = IO::GetResultDesc(sourceResult.error());
            Logger::LogError("Failed to load shader source file {}: {}", args.m_shaderName, resultDesc);

            ShaderCompilerResult failedResult;
            failedResult.m_dependencies.push_back(args.m_shaderName);
            failedResult.m_dependencyHashes.push_back(0);
            return failedResult;
        }

        const Rc<ShaderSourceFile>& sourceFile = sourceResult.value();
        if (!EnsureCompiler())
        {
            ShaderCompilerResult failedResult;
            failedResult.m_dependencies.push_back(args.m_shaderName);
            failedResult.m_dependencyHashes.push_back(sourceFile->GetContentHash());
            return failedResult;
        }
        const festd::string_view source = sourceFile->GetSource();

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = source.data();
        sourceBuffer.Size = source.size();
        sourceBuffer.Encoding = DXC_CP_UTF8;

        Memory::FiberTempAllocator temp;
        festd::pmr::vector<LPCWSTR> compilerArgs{ &temp };
        compilerArgs.reserve(32);

        const IO::PathView shaderName{ args.m_shaderName };
        const festd::string_view shaderFilename = shaderName.filename();
        const Str::Utf8ToUtf16 shaderNameUtf16{ shaderFilename.data(), shaderFilename.size(), &temp };
        compilerArgs.push_back(shaderNameUtf16.ToWideString());

        compilerArgs.push_back(L"-E");
        compilerArgs.push_back(L"main");

        compilerArgs.push_back(L"-I");
        compilerArgs.push_back(L"");

        compilerArgs.push_back(L"-T");
        compilerArgs.push_back(GetShaderTargetProfile(args.m_stage));

        compilerArgs.push_back(L"-HV");
        compilerArgs.push_back(L"2021");

        compilerArgs.push_back(L"-D");
        compilerArgs.push_back(L"FE_PLATFORM_VULKAN");

        for (const auto [m_name, m_value] : args.m_defines)
        {
            const festd::fixed_string defineString = Fmt::FixedFormat("{}={}", m_name, m_value);
            compilerArgs.push_back(L"-D");
            compilerArgs.push_back(ConvertString(&temp, defineString));
        }

        for (const char* arg : kCompilerArgs)
        {
            compilerArgs.push_back(ConvertString(&temp, arg));
        }

        festd::span<const char* const> configArgs = kOptimizedCompilerArgs;
        if (ResolveShaderCompilerConfig(args.m_config) == ShaderCompilerConfig::kDebug)
            configArgs = kDebugCompilerArgs;

        for (const char* arg : configArgs)
        {
            compilerArgs.push_back(ConvertString(&temp, arg));
        }

        Hasher lookupKeyHasher;
        lookupKeyHasher.UpdateRaw(kShaderCacheVersion).UpdateRaw(m_compilerVersionHash).UpdateRaw(args.m_shaderName.GetHash());
        for (const LPCWSTR arg : compilerArgs)
            lookupKeyHasher.Update(arg, wcslen(arg) * sizeof(WCHAR));

        const uint64_t lookupKey = lookupKeyHasher.Finalize();

        ShaderCompilerResult cachedResult;
        if (m_shaderCache.TryLoad(lookupKey, *m_shaderSourceCache, args.m_binaryAllocator, cachedResult))
            return cachedResult;

        const Rc includeHandler = Memory::DefaultNew<DxcIncludeHandler>(m_dxcUtils.Get(), m_shaderSourceCache.Get());
        includeHandler->AddDependency(sourceFile.Get());

        Rc<IDxcResult> result;
        HRESULT hr = m_dxcCompiler->Compile(&sourceBuffer,
                                            compilerArgs.data(),
                                            compilerArgs.size(),
                                            includeHandler.Get(),
                                            IID_PPV_ARGS(result.GetAddressOf()));

        if (FAILED(hr))
        {
            Logger::LogError("Failed to compile shader: {}", args.m_shaderName);
            return MakeFailedResult(includeHandler->GetDependencies());
        }

        {
            Rc<IDxcBlobUtf8> errors;
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
            if (errors.Get() != nullptr && errors->GetStringLength() > 0)
                Logger::LogError("{}: {}", args.m_shaderName, errors->GetStringPointer());
        }

        if (FAILED(result->GetStatus(&hr)) || FAILED(hr))
        {
            Logger::LogError("Failed to compile shader: {}", args.m_shaderName);
            return MakeFailedResult(includeHandler->GetDependencies());
        }

        // Save the PDB
        if (0)
        {
            Rc<IDxcBlob> pdb;
            Rc<IDxcBlobUtf16> pdbFilename;
            hr = result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdb.GetAddressOf()), pdbFilename.GetAddressOf());
            FE_Assert(SUCCEEDED(hr), "Failed to get PDB");

            const Str::Utf16ToUtf8 pdbFilenameUtf8{ pdbFilename->GetStringPointer(),
                                                    static_cast<uint32_t>(pdbFilename->GetStringLength()) };
            IO::Path pdbPath{ "ShaderDebugInfo" };
            pdbPath /= festd::string_view{ pdbFilenameUtf8.data(), pdbFilenameUtf8.size() };

            const Rc pdbStream = IO::FileStream::Open(pdbPath, IO::OpenMode::kTruncate).value();
            pdbStream->WriteFromBuffer(pdb->GetBufferPointer(), pdb->GetBufferSize());
        }

        Rc<IDxcBlob> binary;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(binary.GetAddressOf()), nullptr);
        FE_Assert(SUCCEEDED(hr), "Failed to get binary");
        FE_Assert(binary);

        const uint32_t binarySize = static_cast<uint32_t>(binary->GetBufferSize());
        const festd::span binaryBytes{ static_cast<const std::byte*>(binary->GetBufferPointer()), binarySize };

        // The hash of the bytecode is used so that cached and freshly compiled results are identified the same way.
        ShaderCompilerResult compilerResult;
        compilerResult.m_hash = DefaultHash(binaryBytes.data(), binarySize);
        compilerResult.m_hashValid = true;

        const uint32_t bufferSizeDwordAligned = AlignUp<sizeof(uint32_t)>(binarySize);
        compilerResult.m_byteCode = ByteBuffer(bufferSizeDwordAligned, args.m_binaryAllocator);
        memcpy(compilerResult.m_byteCode.data(), binaryBytes.data(), binarySize);
        memset(compilerResult.m_byteCode.data() + binarySize, 0, bufferSizeDwordAligned - binarySize);

        compilerResult.m_codeValid = true;
        compilerResult.m_byteCodeSize = binarySize;
        AppendDependencies(compilerResult, includeHandler->GetDependencies());

        m_shaderCache.Store(lookupKey, includeHandler->GetDependencies(), binaryBytes);
        return compilerResult;
    }
} // namespace FE::Graphics::Core
