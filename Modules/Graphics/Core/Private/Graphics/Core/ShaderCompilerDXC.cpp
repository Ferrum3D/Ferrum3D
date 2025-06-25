#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/DI/Activator.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/ShaderCompilerDXC.h>

#include <d3d12shader.h>

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
                                                  "-fvk-use-dx-layout",
                                                  "-fspv-reflect",
                                                  "-Od",
                                                  "-Zi",
                                                  "-Zpr" };


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


        struct DxcIncludeHandler final : public IDxcIncludeHandler
        {
            explicit DxcIncludeHandler(IDxcUtils* dxcUtils, ShaderSourceCache* shaderSourceCache, Logger* logger)
                : m_dxcUtils(dxcUtils)
                , m_logger(logger)
                , m_shaderSourceCache(shaderSourceCache)
            {
            }

            HRESULT STDMETHODCALLTYPE LoadSource(const LPCWSTR filenameWide, IDxcBlob** includeSource) override
            {
                FE_PROFILER_ZONE();

                const Str::Utf16ToUtf8 pathUtf8{ filenameWide };
                const IO::Path path = IO::NormalizePath({ pathUtf8.data(), pathUtf8.size() });

                const Env::Name name{ path };
                if (const festd::expected result = m_shaderSourceCache->GetSource(name))
                {
                    const Rc<ShaderSourceFile>& sourceFile = result.value();
                    const festd::string_view source = sourceFile->GetSource();

                    Rc<IDxcBlobEncoding> blobEncoding;
                    const HRESULT hr =
                        m_dxcUtils->CreateBlobFromPinned(source.data(), source.size(), DXC_CP_UTF8, blobEncoding.GetAddressOf());
                    if (SUCCEEDED(hr))
                    {
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
            Logger* m_logger = nullptr;
            ShaderSourceCache* m_shaderSourceCache = nullptr;
            std::atomic<ULONG> m_refCount = 0;
        };
    } // namespace


    ShaderCompilerDXC::ShaderCompilerDXC(Logger* logger, IO::IStreamFactory* streamFactory)
        : m_logger(logger)
        , m_streamFactory(streamFactory)
    {
        FE_PROFILER_ZONE();

        FE_Verify(m_module.Load("dxcompiler"), "Failed to load dxcompiler.dll");

        const auto dxcCreateInstance = m_module.FindFunction<DxcCreateInstanceProc>("DxcCreateInstance");
        FE_Assert(dxcCreateInstance, "Failed to find DxcCreateInstance");

        const HRESULT hrUtils = dxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_dxcUtils.ReleaseAndGetAddressOf()));
        FE_Assert(SUCCEEDED(hrUtils), "Failed to create DXC utils");

        const HRESULT hrCompiler = dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_dxcCompiler.ReleaseAndGetAddressOf()));
        FE_Assert(SUCCEEDED(hrCompiler), "Failed to create DXC compiler");

        m_shaderSourceCache = DI::DefaultNew<ShaderSourceCache>().value();
        m_dxcIncludeHandler = Rc<DxcIncludeHandler>::DefaultNew(m_dxcUtils.Get(), m_shaderSourceCache.Get(), logger);
    }


    ShaderCompilerResult ShaderCompilerDXC::CompileShader(const ShaderCompilerArgs& args)
    {
        FE_PROFILER_ZONE();

        const festd::expected sourceResult = m_shaderSourceCache->GetSource(args.m_shaderName);
        if (!sourceResult)
        {
            const festd::string_view resultDesc = IO::GetResultDesc(sourceResult.error());
            m_logger->LogError("Failed to load shader source file {}: {}", args.m_shaderName, resultDesc);
            return {};
        }

        const Rc<ShaderSourceFile>& sourceFile = sourceResult.value();
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

        Rc<IDxcResult> result;
        HRESULT hr = m_dxcCompiler->Compile(&sourceBuffer,
                                            compilerArgs.data(),
                                            compilerArgs.size(),
                                            m_dxcIncludeHandler.Get(),
                                            IID_PPV_ARGS(result.GetAddressOf()));

        if (FAILED(hr))
        {
            m_logger->LogError("Failed to compile shader: {}", args.m_shaderName);
            return {};
        }

        {
            Rc<IDxcBlobUtf8> errors;
            result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
            if (errors.Get() != nullptr && errors->GetStringLength() > 0)
                m_logger->LogError("{}: {}", args.m_shaderName, errors->GetStringPointer());
        }

        if (FAILED(result->GetStatus(&hr)) || FAILED(hr))
        {
            m_logger->LogError("Failed to compile shader: {}", args.m_shaderName);
            return {};
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

            const Rc pdbStream = m_streamFactory->OpenFileStream(pdbPath, IO::OpenMode::kTruncate).value();
            pdbStream->WriteFromBuffer(pdb->GetBufferPointer(), pdb->GetBufferSize());
        }

        ShaderCompilerResult compilerResult;

        // Get hash
        {
            Rc<IDxcBlob> hash;
            hr = result->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(hash.GetAddressOf()), nullptr);

            if (SUCCEEDED(hr) && hash)
            {
                const auto* hashData = static_cast<DxcShaderHash*>(hash->GetBufferPointer());
                compilerResult.m_hash = DefaultHash(hashData->HashDigest, sizeof(hashData->HashDigest));
                compilerResult.m_hashValid = true;
            }
            else
            {
                m_logger->LogError("Failed to get shader hash: {}", args.m_shaderName);
            }
        }

        Rc<IDxcBlob> binary;
        hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(binary.GetAddressOf()), nullptr);
        FE_Assert(SUCCEEDED(hr), "Failed to get binary");
        FE_Assert(binary);

        const uint32_t binarySize = static_cast<uint32_t>(binary->GetBufferSize());
        const uint32_t bufferSizeDwordAligned = AlignUp<sizeof(uint32_t)>(binarySize);
        compilerResult.m_byteCode = ByteBuffer(bufferSizeDwordAligned, args.m_binaryAllocator);
        memcpy(compilerResult.m_byteCode.data(), binary->GetBufferPointer(), binarySize);
        memset(compilerResult.m_byteCode.data() + binarySize, 0, bufferSizeDwordAligned - binarySize);

        compilerResult.m_codeValid = true;
        compilerResult.m_byteCodeSize = binarySize;

        return compilerResult;
    }
} // namespace FE::Graphics::Core
