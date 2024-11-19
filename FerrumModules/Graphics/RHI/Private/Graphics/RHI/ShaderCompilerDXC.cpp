#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Logging/Trace.h>
#include <Graphics/RHI/ShaderCompilerDXC.h>
#include <d3d12shader.h>

FE_PUSH_MSVC_WARNING(4244)

namespace FE::Graphics::RHI
{
    class IncludeHandler : public IDxcIncludeHandler
    {
        std::atomic<int32_t> m_refCounter;
        std::pmr::wstring m_basePath; // TODO: remove this
        IDxcLibrary* m_library;

    public:
        inline IncludeHandler(const std::pmr::wstring& basePath, IDxcLibrary* library)
            : m_basePath(basePath)
            , m_library(library)
            , m_refCounter(0)
        {
        }

        inline HRESULT QueryInterface(const IID&, void**) override
        {
            return E_FAIL;
        }

        inline ULONG AddRef() override
        {
            return ++m_refCounter;
        }

        inline ULONG Release() override
        {
            return --m_refCounter;
        }

        inline HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
        {
            auto path = m_basePath + pFilename;
            CComPtr<IDxcBlobEncoding> source;
            HRESULT result = m_library->CreateBlobFromFile(path.c_str(), nullptr, &source);

            if (SUCCEEDED(result) && ppIncludeSource)
                *ppIncludeSource = source.Detach();
            return result;
        }
    };

    inline static std::pmr::wstring GetTargetProfile(ShaderStage stage, HLSLShaderVersion version)
    {
        std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, Memory::StdDefaultAllocator<wchar_t>> result;
        switch (stage)
        {
        case ShaderStage::kVertex:
            result << L"vs_";
            break;
        case ShaderStage::kPixel:
            result << L"ps_";
            break;
        case ShaderStage::kHull:
            result << L"hs_";
            break;
        case ShaderStage::kDomain:
            result << L"ds_";
            break;
        case ShaderStage::kGeometry:
            result << L"gs_";
            break;
        case ShaderStage::kCompute:
            result << L"cs_";
            break;
        }

        result << version.m_major << L'_' << version.m_minor;
        return std::pmr::wstring{ result.str() };
    }

    ShaderCompilerDXC::ShaderCompilerDXC(Logger* logger, GraphicsAPI api)
        : m_api(api)
        , m_logger(logger)
    {
        m_module.Load("dxcompiler");
    }

    ByteBuffer ShaderCompilerDXC::CompileShader(const ShaderCompilerArgs& args)
    {
        ZoneScoped;

        auto sepIter = args.m_fullPath.FindLastOf('/');
        const std::pmr::wstring shaderName(sepIter + 1, args.m_fullPath.end());
        const std::pmr::wstring baseDirectory(args.m_fullPath.begin(), sepIter);

        auto createInstance = m_module.FindFunction<DxcCreateInstanceProc>("DxcCreateInstance");

        CComPtr<IDxcLibrary> library;
        HRESULT result = createInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
        if (FAILED(result))
        {
            m_logger->LogError("Couldn't create a DXC library");
            return {};
        }

        CComPtr<IDxcCompiler> compiler;
        result = createInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(result))
        {
            m_logger->LogError("Couldn't create a DXC compiler");
            return {};
        }

        CComPtr<IDxcBlobEncoding> source;
        auto sourceSize = static_cast<uint32_t>(args.m_sourceCode.Size());
        result = library->CreateBlobWithEncodingFromPinned(args.m_sourceCode.Data(), sourceSize, CP_UTF8, &source);
        if (FAILED(result))
        {
            m_logger->LogError("Couldn't create a DXC Blob encoding");
            return {};
        }

        IncludeHandler includeHandler(baseDirectory, library);

        festd::vector<DxcDefine> defines;
        defines.push_back(DxcDefine{ L"FE_VULKAN", L"1" });

#if FE_DEBUG
        defines.push_back(DxcDefine{ L"FE_DEBUG", L"1" });
#else
        defines.push_back(DxcDefine{ L"FE_DEBUG", L"0" });
#endif
        auto defineCount = static_cast<uint32_t>(defines.size());

        festd::vector<LPCWSTR> compileArgs{ L"-O3", L"-Zpc" };
        if (m_api == GraphicsAPI::kVulkan)
        {
            compileArgs.push_back(L"-spirv");
            // TODO: for some reason vulkan1.2 doesn't work, validation layers say:
            // "Invalid SPIR-V binary version 1.5 for target environment SPIR-V 1.3 (under Vulkan 1.1 semantics)"
            compileArgs.push_back(L"-fspv-target-env=vulkan1.1");
            compileArgs.push_back(L"-fspv-extension=KHR");
            compileArgs.push_back(L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1");
            compileArgs.push_back(L"-fspv-extension=SPV_GOOGLE_user_type");
            compileArgs.push_back(L"-fvk-use-dx-layout");
            compileArgs.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
            compileArgs.push_back(L"-fspv-reflect");
            compileArgs.push_back(L"-Od");
        }

        const uint32_t argsCount = compileArgs.size();

        const StringSlice entryPointView{ args.m_entryPoint };
        const std::pmr::wstring entryPoint{ entryPointView.begin(), entryPointView.end() };
        const std::pmr::wstring profile = GetTargetProfile(args.m_stage, args.m_version);
        CComPtr<IDxcOperationResult> compileResult;
        result = compiler->Compile(source,
                                   shaderName.c_str(),
                                   entryPoint.c_str(),
                                   profile.c_str(),
                                   compileArgs.data(),
                                   argsCount,
                                   defines.data(),
                                   defineCount,
                                   &includeHandler,
                                   &compileResult);
        if (SUCCEEDED(result))
        {
            HRESULT status;
            if (SUCCEEDED(compileResult->GetStatus(&status)))
            {
                result = status;
            }
        }

        festd::vector<uint8_t> returnValue;
        if (SUCCEEDED(result))
        {
            CComPtr<IDxcBlob> byteCode;
            FE_Assert(SUCCEEDED(compileResult->GetResult(&byteCode)));
            auto bufferPtr = static_cast<uint8_t*>(byteCode->GetBufferPointer());
            returnValue.assign(bufferPtr, bufferPtr + byteCode->GetBufferSize());
        }
        else
        {
            CComPtr<IDxcBlobEncoding> errors;
            CComPtr<IDxcBlobEncoding> unicodeErrors;
            if (SUCCEEDED(compileResult->GetErrorBuffer(&errors)) && SUCCEEDED(library->GetBlobAsUtf8(errors, &unicodeErrors)))
            {
                auto errorString = static_cast<const char*>(unicodeErrors->GetBufferPointer());
                m_logger->LogError("Shader compilation failed: {}", StringSlice(errorString));
            }
        }

        return ByteBuffer(std::move(returnValue));
    }
} // namespace FE::Graphics::RHI

FE_POP_MSVC_WARNING
