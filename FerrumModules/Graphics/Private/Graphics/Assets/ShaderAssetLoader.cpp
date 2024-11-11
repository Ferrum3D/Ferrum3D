#include <FeCore/DI/Activator.h>
#include <FeCore/Utils/BinarySerializer.h>
#include <Graphics/Assets/ShaderAssetLoader.h>
#include <Graphics/Assets/ShaderAssetStorage.h>
#include <HAL/ShaderCompiler.h>

namespace FE::Graphics
{
    struct ShaderAssetLoader::CompilerJob : public Job
    {
        IO::FixedPath m_path;
        StringSlice m_sourceCode;
        ShaderAssetStorage* m_storage = nullptr;
        ShaderAssetLoader* m_loader = nullptr;
        HAL::ShaderCompiler* m_compiler = nullptr;
        Rc<HAL::ShaderModule> m_shaderModule;
        Logger* m_logger = nullptr;

        inline CompilerJob(HAL::ShaderCompiler* compiler, HAL::ShaderModule* shaderModule, Logger* logger)
            : m_compiler(compiler)
            , m_shaderModule(shaderModule)
            , m_logger(logger)
        {
        }

        inline void Execute() override
        {
            ZoneScoped;

            HAL::ShaderCompilerArgs compilerArgs;
            compilerArgs.EntryPoint = "main";
            compilerArgs.SourceCode = m_sourceCode;
            compilerArgs.FullPath = m_path;
            compilerArgs.Version = HAL::HLSLShaderVersion{ 6, 1 };

            const StringSlice pathNoExt{ m_path.begin(), m_path.FindLastOf('.') };
            if (pathNoExt.EndsWith(".ps"))
                compilerArgs.Stage = HAL::ShaderStage::kPixel;
            else if (pathNoExt.EndsWith(".vs"))
                compilerArgs.Stage = HAL::ShaderStage::kVertex;
            else if (pathNoExt.EndsWith(".cs"))
                compilerArgs.Stage = HAL::ShaderStage::kCompute;
            else if (pathNoExt.EndsWith(".gs"))
                compilerArgs.Stage = HAL::ShaderStage::kGeometry;
            else if (pathNoExt.EndsWith(".hs"))
                compilerArgs.Stage = HAL::ShaderStage::kHull;
            else if (pathNoExt.EndsWith(".ds"))
                compilerArgs.Stage = HAL::ShaderStage::kDomain;

            const ByteBuffer byteCode = m_compiler->CompileShader(compilerArgs);
            if (byteCode.size() == 0)
            {
                m_logger->LogError("Shader compilation failed {}", m_path);
            }
            else
            {
                // TODO: save to a SPIR-V file

                HAL::ShaderModuleDesc moduleDesc;
                moduleDesc.ByteCode = byteCode;
                moduleDesc.EntryPoint = compilerArgs.EntryPoint;
                moduleDesc.Stage = compilerArgs.Stage;
                FE_Verify(m_shaderModule->Init(moduleDesc) == HAL::ResultCode::Success);
                m_storage->m_ShaderModule = m_shaderModule;
                m_storage->m_Ready.store(true, std::memory_order_release);
            }

            Memory::DefaultFree((void*)m_sourceCode.Data());
            Memory::Delete(&m_loader->m_CompilerJobPool, this, sizeof(CompilerJob));
        }
    };


    void ShaderAssetLoader::AsyncIOCallback(const IO::AsyncReadResult& result)
    {
        ZoneScoped;

        FE_Assert(result.pController->GetStatus() == IO::AsyncOperationStatus::kSucceeded);

        CompilerJob* job = DI::New<CompilerJob>(&m_CompilerJobPool).Unwrap();
        job->m_path = result.pRequest->Path;
        job->m_sourceCode = { reinterpret_cast<const char*>(result.pRequest->pReadBuffer), result.pRequest->ReadBufferSize };
        job->m_storage = static_cast<ShaderAssetStorage*>(result.pRequest->pUserData);
        job->m_loader = this;
        job->Schedule(m_pJobSystem);
    }


    ShaderAssetLoader::ShaderAssetLoader(IO::IStreamFactory* pStreamFactory, IO::IAsyncStreamIO* pAsyncIO, IJobSystem* pJobSystem)
        : m_CompilerJobPool("ShaderAssetLoader/CompilerJob", sizeof(CompilerJob), 64 * 1024)
        , m_pStreamFactory(pStreamFactory)
        , m_pAsyncStreamIO(pAsyncIO)
        , m_pJobSystem(pJobSystem)
    {
        m_SourceExtensions.push_back(".hlsl");
        m_SourceExtensions.push_back(".hlsli");

        m_Spec.AssetTypeName = Env::Name{ ShaderAssetStorage::kAssetTypeName };
        m_Spec.FileExtension = ".spv";
        m_Spec.SourceExtensions = m_SourceExtensions;
    }


    const Assets::AssetLoaderSpec& ShaderAssetLoader::GetSpec() const
    {
        return m_Spec;
    }


    void ShaderAssetLoader::CreateStorage(Assets::AssetStorage** ppStorage)
    {
        *ppStorage = Rc<ShaderAssetStorage>::DefaultNew(this);
        (*ppStorage)->AddStrongRef();
    }


    void ShaderAssetLoader::LoadAsset(Assets::AssetStorage* storage, Env::Name assetName)
    {
        ZoneScoped;

        const IO::FixedPath spvFilename = IO::FixedPath{ assetName } + m_Spec.FileExtension;
        // TODO: check if already compiled
        // if (m_pStreamFactory->FileExists(spvFilename))
        // {
        //     const Rc<IO::IStream> spvFile = m_pStreamFactory->OpenFileStream(spvFilename, IO::OpenMode::kReadWrite).Unwrap();
        //     const IO::FileStats stats = spvFile->GetStats();
        // }

        for (const StringSlice extension : m_Spec.SourceExtensions)
        {
            const IO::FixedPath path = IO::FixedPath{ assetName } + extension;
            if (!m_pStreamFactory->FileExists(path))
                continue;

            IO::AsyncReadRequest request;
            request.Path = path;
            request.pCallback = this;
            request.Priority = IO::Priority::kHigh;
            request.pUserData = storage;
            m_pAsyncStreamIO->ReadAsync(request);
            return;
        }

        Trace::ReportCritical("File not found");
    }
} // namespace FE::Graphics
