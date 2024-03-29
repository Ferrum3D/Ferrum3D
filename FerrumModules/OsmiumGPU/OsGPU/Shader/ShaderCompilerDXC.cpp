#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <OsGPU/Shader/ShaderCompilerDXC.h>
#include <d3d12shader.h>

namespace FE::Osmium
{
    class IncludeHandler : public IDxcIncludeHandler
    {
        AtomicInt32 m_RefCounter;
        WString m_BasePath;
        IDxcLibrary* m_Library;

    public:
        inline IncludeHandler(const WString& basePath, IDxcLibrary* library)
            : m_BasePath(basePath)
            , m_Library(library)
            , m_RefCounter(0)
        {
        }

        inline HRESULT QueryInterface(const IID&, void**) override
        {
            return E_FAIL;
        }

        inline ULONG AddRef() override
        {
            return Interlocked::Increment(m_RefCounter);
        }

        inline ULONG Release() override
        {
            return Interlocked::Decrement(m_RefCounter);
        }

        inline HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
        {
            auto path = m_BasePath + pFilename;
            CComPtr<IDxcBlobEncoding> source;
            HRESULT result = m_Library->CreateBlobFromFile(path.c_str(), nullptr, &source);

            if (SUCCEEDED(result) && ppIncludeSource)
                *ppIncludeSource = source.Detach();
            return result;
        }
    };

    inline WString GetTargetProfile(ShaderStage stage, HLSLShaderVersion version)
    {
        std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, StdHeapAllocator<wchar_t>> result;
        switch (stage)
        {
        case ShaderStage::Vertex:
            result << L"vs_";
            break;
        case ShaderStage::Pixel:
            result << L"ps_";
            break;
        case ShaderStage::Hull:
            result << L"hs_";
            break;
        case ShaderStage::Domain:
            result << L"ds_";
            break;
        case ShaderStage::Geometry:
            result << L"gs_";
            break;
        case ShaderStage::Compute:
            result << L"cs_";
            break;
        }

        result << version.Major << L'_' << version.Minor;
        return result.str();
    }

    ShaderCompilerDXC::ShaderCompilerDXC(GraphicsAPI api)
        : m_API(api)
    {
        m_Module.LoadFrom("dxcompiler");
    }

    ByteBuffer ShaderCompilerDXC::CompileShader(const ShaderCompilerArgs& args)
    {
        auto sepIter       = args.FullPath.FindLastOf('/');
        auto shaderName    = StringSlice(sepIter + 1, args.FullPath.end()).ToWideString();
        auto baseDirectory = StringSlice(args.FullPath.begin(), sepIter).ToWideString();

        auto createInstance = m_Module.GetFunction<DxcCreateInstanceProc>("DxcCreateInstance");

        CComPtr<IDxcLibrary> library;
        HRESULT result = createInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
        if (FAILED(result))
        {
            FE_LOG_ERROR("Couldn't create a DXC library");
            return {};
        }

        CComPtr<IDxcCompiler> compiler;
        result = createInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(result))
        {
            FE_LOG_ERROR("Couldn't create a DXC compiler");
            return {};
        }

        CComPtr<IDxcBlobEncoding> source;
        auto sourceSize = static_cast<UInt32>(args.SourceCode.Size());
        result          = library->CreateBlobWithEncodingFromPinned(args.SourceCode.Data(), sourceSize, CP_UTF8, &source);
        if (FAILED(result))
        {
            FE_LOG_ERROR("Couldn't create a DXC Blob encoding");
            return {};
        }

        IncludeHandler includeHandler(baseDirectory, library);

        List<DxcDefine> defines;
        defines.Push(DxcDefine{ L"FE_VULKAN", L"1" });

#if FE_DEBUG
        defines.Push(DxcDefine{ L"FE_DEBUG", L"1" });
#else
        defines.Push(DxcDefine{ L"FE_DEBUG", L"0" });
#endif
        auto defineCount = static_cast<UInt32>(defines.Size());

        List<LPCWSTR> compileArgs{ L"-O3", L"-Zpc" };
        if (m_API == GraphicsAPI::Vulkan)
        {
            compileArgs
                .Append(L"-spirv")
                // TODO: for some reason vulkan1.2 doesn't work, validation layers say:
                // "Invalid SPIR-V binary version 1.5 for target environment SPIR-V 1.3 (under Vulkan 1.1 semantics)"
                .Append(L"-fspv-target-env=vulkan1.1")
                .Append(L"-fspv-extension=KHR")
                .Append(L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1")
                .Append(L"-fspv-extension=SPV_GOOGLE_user_type")
                .Append(L"-fvk-use-dx-layout")
                .Append(L"-fspv-extension=SPV_EXT_descriptor_indexing")
                .Append(L"-fspv-reflect")
                .Append(L"-Od");
        }
        auto argsCount = static_cast<UInt32>(compileArgs.Size());

        auto entryPoint = args.EntryPoint.ToWideString();
        auto profile    = GetTargetProfile(args.Stage, args.Version);
        CComPtr<IDxcOperationResult> compileResult;
        result = compiler->Compile(source,
                                   shaderName.c_str(),
                                   entryPoint.c_str(),
                                   profile.c_str(),
                                   compileArgs.Data(),
                                   argsCount,
                                   defines.Data(),
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

        List<UInt8> returnValue;
        if (SUCCEEDED(result))
        {
            CComPtr<IDxcBlob> byteCode;
            FE_ASSERT(SUCCEEDED(compileResult->GetResult(&byteCode)));
            auto bufferPtr = static_cast<UInt8*>(byteCode->GetBufferPointer());
            returnValue.Assign(bufferPtr, bufferPtr + byteCode->GetBufferSize());
        }
        else
        {
            CComPtr<IDxcBlobEncoding> errors;
            CComPtr<IDxcBlobEncoding> unicodeErrors;
            if (SUCCEEDED(compileResult->GetErrorBuffer(&errors)) && SUCCEEDED(library->GetBlobAsUtf8(errors, &unicodeErrors)))
            {
                auto errorString = static_cast<const char*>(unicodeErrors->GetBufferPointer());
                FE_LOG_ERROR("Shader compilation failed: {}", String(errorString));
            }
        }

        return ByteBuffer(std::move(returnValue));
    }
} // namespace FE::Osmium
