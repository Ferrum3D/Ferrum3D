#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Jobs/Job.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/ShaderLibrary.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        Env::Name GetShaderEntryPointName(const RHI::ShaderStage shaderStage)
        {
            switch (shaderStage)
            {
            case RHI::ShaderStage::kVertex:
                return "vs_main";
            case RHI::ShaderStage::kPixel:
                return "ps_main";
            case RHI::ShaderStage::kHull:
                return "hs_main";
            case RHI::ShaderStage::kDomain:
                return "ds_main";
            case RHI::ShaderStage::kGeometry:
                return "gs_main";
            case RHI::ShaderStage::kCompute:
                return "main";

            default:
                FE_DebugBreak();
                return Env::Name::kEmpty;
            }
        }
    } // namespace


    struct ShaderLibrary::TaskImplementation final
        : public IO::IAsyncReadCallback
        , public Job
    {
        void Execute() override;
        void AsyncIOCallback(const IO::AsyncReadResult& result) override;

        ShaderLibrary* m_parent = nullptr;
        std::pmr::memory_resource* m_sourceCodeAllocator = nullptr;
        char* m_sourceCode = nullptr;
        uint32_t m_sourceCodeLength = 0;
        uint32_t m_shaderIndex = kInvalidIndex;
    };


    void ShaderLibrary::TaskImplementation::Execute()
    {
        FE_PROFILER_FUNCTION();

        ShaderInfo& shaderInfo = m_parent->m_shaders[m_shaderIndex];

        RHI::ShaderCompilerArgs compilerArgs;
        compilerArgs.m_stage = shaderInfo.m_permutationDesc.m_stage;
        compilerArgs.m_version = m_parent->m_hlslVersion;
        compilerArgs.m_entryPoint = shaderInfo.m_entryPoint;
        compilerArgs.m_sourceCode = { m_sourceCode, m_sourceCodeLength };
        compilerArgs.m_fullPath = shaderInfo.m_permutationDesc.m_name;

        ByteBuffer byteCode = m_parent->m_shaderCompiler->CompileShader(compilerArgs);

        m_sourceCodeAllocator->deallocate(m_sourceCode, m_sourceCodeLength);
        m_sourceCode = nullptr;

        festd::vector<uint32_t> alignedByteCode;
        alignedByteCode.resize(Math::CeilDivide(byteCode.size(), 4));
        memcpy(alignedByteCode.data(), byteCode.data(), byteCode.size());

        const VkDevice device = NativeCast(m_parent->m_device);

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = byteCode.size();
        shaderModuleCI.pCode = alignedByteCode.data();
        FE_VK_ASSERT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderInfo.m_shaderModule));

        shaderInfo.m_loaded.store(true, std::memory_order_release);

        Memory::Delete(&m_parent->m_taskPool, this, sizeof(TaskImplementation));
    }


    void ShaderLibrary::TaskImplementation::AsyncIOCallback(const IO::AsyncReadResult& result)
    {
        FE_PROFILER_FUNCTION();

        switch (result.m_controller->GetStatus())
        {
        case IO::AsyncOperationStatus::kFailed:
            m_parent->m_logger->LogError("Failed to read shader file {}", result.m_request->m_path);
            [[fallthrough]];

        case IO::AsyncOperationStatus::kCanceled:
            result.FreeData();
            Memory::Delete(&m_parent->m_taskPool, this, sizeof(TaskImplementation));
            return;

        case IO::AsyncOperationStatus::kSucceeded:
            break;

        default:
            FE_DebugBreak();
            break;
        }

        m_sourceCode = reinterpret_cast<char*>(result.m_request->m_readBuffer);
        m_sourceCodeLength = result.m_request->m_readBufferSize;
        m_sourceCodeAllocator = result.m_request->m_allocator;

        Schedule(Env::GetServiceProvider()->ResolveRequired<IJobSystem>(), nullptr, JobPriority::kBackground);
    }


    ShaderLibrary::ShaderLibrary(RHI::ShaderCompiler* shaderCompiler, Logger* logger, IO::IAsyncStreamIO* asyncIO)
        : m_taskPool("RHI/ShaderLibrary/TaskPool", sizeof(TaskImplementation))
        , m_shaderCompiler(shaderCompiler)
        , m_logger(logger)
        , m_asyncIO(asyncIO)
    {
    }


    RHI::ShaderHandle ShaderLibrary::GetShader(const RHI::ShaderPermutationDesc& desc)
    {
        const uint64_t hash = desc.GetHash();
        const uint32_t shaderIndex = m_shaders.size();
        const auto [iter, inserted] = m_shadersMap.insert({ hash, shaderIndex });
        if (!inserted)
        {
#if FE_DEBUG
            const ShaderInfo& shaderInfo = m_shaders[iter->second];
            FE_Assert(shaderInfo.m_permutationDesc.m_name == desc.m_name);
            FE_Assert(shaderInfo.m_permutationDesc.m_stage == desc.m_stage);

            for (uint32_t defineIndex = 0; defineIndex < desc.m_defines.size(); ++defineIndex)
            {
                FE_Assert(desc.m_defines[defineIndex].m_name == shaderInfo.m_permutationDesc.m_defines[defineIndex].m_name);
                FE_Assert(desc.m_defines[defineIndex].m_value == shaderInfo.m_permutationDesc.m_defines[defineIndex].m_value);
            }
#endif

            return RHI::ShaderHandle{ iter->second };
        }

        auto* definesCopy = Memory::AllocateArray<RHI::ShaderDefine>(&m_shaderInfoAllocator, desc.m_defines.size());
        for (uint32_t defineIndex = 0; defineIndex < desc.m_defines.size(); ++defineIndex)
        {
            const auto& [srcName, srcValue] = desc.m_defines[defineIndex];
            auto& [dstName, dstValue] = definesCopy[defineIndex];

            dstName = festd::string_view::Copy(&m_shaderInfoAllocator, srcName);
            dstValue = festd::string_view::Copy(&m_shaderInfoAllocator, srcValue);
        }

        ShaderInfo& shaderInfo = m_shaders.push_back();
        shaderInfo.m_permutationDesc = desc;
        shaderInfo.m_entryPoint = GetShaderEntryPointName(desc.m_stage);
        shaderInfo.m_permutationDesc.m_defines = { definesCopy, desc.m_defines.size() };

        auto* task = Memory::New<TaskImplementation>(&m_taskPool);
        task->m_parent = this;
        task->m_shaderIndex = shaderIndex;

        IO::AsyncReadRequest request;
        request.m_path = festd::string_view{ desc.m_name };
        request.m_callback = task;
        m_asyncIO->ReadAsync(request);

        return RHI::ShaderHandle{ shaderIndex };
    }
} // namespace FE::Graphics::Vulkan
