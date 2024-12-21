#include <FeCore/DI/Activator.h>
#include <FeCore/Utils/BinarySerializer.h>
#include <Graphics/Assets/ShaderAssetLoader.h>
#include <Graphics/Assets/ShaderAssetStorage.h>
#include <Graphics/RHI/ShaderCompiler.h>

namespace FE::Graphics
{
    struct ShaderAssetLoader::CompilerJob : public Job
    {
        IO::FixedPath m_path;
        StringSlice m_sourceCode;
        ShaderAssetStorage* m_storage = nullptr;
        ShaderAssetLoader* m_loader = nullptr;
        RHI::ShaderCompiler* m_compiler = nullptr;
        Rc<RHI::ShaderModule> m_shaderModule;
        Logger* m_logger = nullptr;

        CompilerJob(RHI::ShaderCompiler* compiler, RHI::ShaderModule* shaderModule, Logger* logger)
            : m_compiler(compiler)
            , m_shaderModule(shaderModule)
            , m_logger(logger)
        {
        }

        void Execute() override
        {
            ZoneScoped;

            RHI::ShaderCompilerArgs compilerArgs;
            compilerArgs.m_entryPoint = "main";
            compilerArgs.m_sourceCode = m_sourceCode;
            compilerArgs.m_fullPath = m_path;
            compilerArgs.m_version = RHI::HLSLShaderVersion{ 6, 1 };

            const StringSlice pathNoExt{ m_path.begin(), m_path.FindLastOf('.') };
            if (pathNoExt.EndsWith(".ps"))
                compilerArgs.m_stage = RHI::ShaderStage::kPixel;
            else if (pathNoExt.EndsWith(".vs"))
                compilerArgs.m_stage = RHI::ShaderStage::kVertex;
            else if (pathNoExt.EndsWith(".cs"))
                compilerArgs.m_stage = RHI::ShaderStage::kCompute;
            else if (pathNoExt.EndsWith(".gs"))
                compilerArgs.m_stage = RHI::ShaderStage::kGeometry;
            else if (pathNoExt.EndsWith(".hs"))
                compilerArgs.m_stage = RHI::ShaderStage::kHull;
            else if (pathNoExt.EndsWith(".ds"))
                compilerArgs.m_stage = RHI::ShaderStage::kDomain;

            const ByteBuffer byteCode = m_compiler->CompileShader(compilerArgs);
            if (byteCode.size() == 0)
            {
                m_logger->LogError("Shader compilation failed {}", m_path);
            }
            else
            {
                // TODO: save to a SPIR-V file

                RHI::ShaderModuleDesc moduleDesc;
                moduleDesc.m_byteCode = byteCode;
                moduleDesc.m_entryPoint = compilerArgs.m_entryPoint;
                moduleDesc.m_stage = compilerArgs.m_stage;
                FE_Verify(m_shaderModule->Init(moduleDesc) == RHI::ResultCode::kSuccess);
                m_storage->m_shaderModule = m_shaderModule;
                m_storage->m_ready.store(true, std::memory_order_release);
            }

            Memory::DefaultFree((void*)m_sourceCode.Data());
            Memory::Delete(&m_loader->m_compilerJobPool, this, sizeof(CompilerJob));
        }
    };


    void ShaderAssetLoader::AsyncIOCallback(const IO::AsyncReadResult& result)
    {
        ZoneScoped;

        FE_Assert(result.pController->GetStatus() == IO::AsyncOperationStatus::kSucceeded);

        CompilerJob* job = DI::New<CompilerJob>(&m_compilerJobPool).Unwrap();
        job->m_path = result.pRequest->Path;
        job->m_sourceCode = { reinterpret_cast<const char*>(result.pRequest->pReadBuffer), result.pRequest->ReadBufferSize };
        job->m_storage = static_cast<ShaderAssetStorage*>(result.pRequest->pUserData);
        job->m_loader = this;
        job->Schedule(m_jobSystem);
    }


    ShaderAssetLoader::ShaderAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem)
        : m_compilerJobPool("ShaderAssetLoader/CompilerJob", sizeof(CompilerJob), 64 * 1024)
        , m_streamFactory(pStreamFactory)
        , m_asyncStreamIO(pAsyncIO)
        , m_jobSystem(pJobSystem)
    {
        m_sourceExtensions.push_back(".hlsl");
        m_sourceExtensions.push_back(".hlsli");

        m_spec.m_assetTypeName = Env::Name{ ShaderAssetStorage::kAssetTypeName };
        m_spec.m_fileExtension = ".spv";
        m_spec.m_sourceExtensions = m_sourceExtensions;
    }


    const Assets::AssetLoaderSpec& ShaderAssetLoader::GetSpec() const
    {
        return m_spec;
    }


    void ShaderAssetLoader::CreateStorage(Assets::AssetStorage** ppStorage)
    {
        *ppStorage = Rc<ShaderAssetStorage>::DefaultNew(this);
        (*ppStorage)->AddStrongRef();
    }


    void ShaderAssetLoader::LoadAsset(Assets::AssetStorage* storage, Env::Name assetName)
    {
        ZoneScoped;

        const IO::FixedPath spvFilename = IO::FixedPath{ assetName } + m_spec.m_fileExtension;
        // TODO: check if already compiled
        // if (m_streamFactory->FileExists(spvFilename))
        // {
        //     const Rc<IO::IStream> spvFile = m_streamFactory->OpenFileStream(spvFilename, IO::OpenMode::kReadWrite).Unwrap();
        //     const IO::FileStats stats = spvFile->GetStats();
        // }

        for (const StringSlice extension : m_spec.m_sourceExtensions)
        {
            const IO::FixedPath path = IO::FixedPath{ assetName } + extension;
            if (!m_streamFactory->FileExists(path))
                continue;

            IO::AsyncReadRequest request;
            request.Path = path;
            request.pCallback = this;
            request.Priority = IO::Priority::kHigh;
            request.pUserData = storage;
            m_asyncStreamIO->ReadAsync(request);
            return;
        }

        Trace::ReportCritical("File not found");
    }
} // namespace FE::Graphics
