#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Jobs/Job.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderLibrary::CompilationTask final : public Job
    {
        void Execute() override;

        ShaderLibrary* m_parent = nullptr;
        uint32_t m_shaderIndex = kInvalidIndex;
    };


    void ShaderLibrary::CompilationTask::Execute()
    {
        ShaderInfo* shaderInfo = m_parent->m_shaders[m_shaderIndex];

        FE_PROFILER_ZONE_TEXT(shaderInfo->m_name.c_str());

        auto defer = festd::defer([this] {
            Memory::Delete(&m_parent->m_taskPool, this, sizeof(*this));
        });

        Core::ShaderCompiler* compiler = m_parent->m_shaderCompiler;

        // TODO: temp allocator
        const auto definesList = shaderInfo->m_definesStorage.ToVector();

        Core::ShaderCompilerArgs args;
        args.m_shaderName = shaderInfo->m_name;
        args.m_stage = shaderInfo->m_stage;
        args.m_defines = definesList;

        const Core::ShaderCompilerResult result = compiler->CompileShader(args);
        if (!result.m_codeValid)
            return;

        FE_AssertDebug(IsAlignedPtr(result.m_byteCode.data(), sizeof(uint32_t)));

        const VkDevice device = NativeCast(m_parent->m_device);

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = result.m_byteCodeSize;
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(result.m_byteCode.data());
        VerifyVulkan(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderInfo->m_shaderModule));

        const festd::span byteCode{ shaderModuleCI.pCode,
                                    shaderModuleCI.pCode + Math::CeilDivide(shaderModuleCI.codeSize, sizeof(uint32_t)) };
        shaderInfo->m_reflection = Rc<ShaderReflection>::New(m_parent->m_reflectionPool.GetAllocator(), byteCode);
    }


    ShaderLibrary::ShaderLibrary(Core::Device* device, Core::ShaderCompiler* shaderCompiler, IJobSystem* jobSystem)
        : m_taskPool("Graphics/Core/ShaderLibrary/TaskPool", sizeof(CompilationTask))
        , m_shaderPool("Graphics/Core/ShaderLibrary/ShaderInfoPool")
        , m_reflectionPool("Graphics/Core/ShaderLibrary/ShaderReflectionPool")
        , m_shaderCompiler(shaderCompiler)
        , m_jobSystem(jobSystem)
    {
        m_device = device;
    }


    ShaderLibrary::~ShaderLibrary()
    {
        for (const ShaderInfo* shaderInfo : m_shaders)
        {
            m_shaderPool.Delete(shaderInfo);
        }
    }


    Core::ShaderHandle ShaderLibrary::GetShader(const Core::ShaderPermutationDesc& desc)
    {
        FE_PROFILER_ZONE();

        const uint64_t hash = desc.GetHash();
        const uint32_t shaderIndex = m_shaders.size();
        const auto [iter, inserted] = m_shadersMap.insert({ hash, shaderIndex });
        if (!inserted)
        {
            if (Build::IsDebug())
            {
                const ShaderInfo* shaderInfo = m_shaders[iter->second];
                FE_Assert(shaderInfo->m_name == desc.m_name);
                FE_Assert(shaderInfo->m_stage == desc.m_stage);

                for (uint32_t defineIndex = 0; defineIndex < desc.m_defines.size(); ++defineIndex)
                {
                    const Core::ShaderDefine define = shaderInfo->m_definesStorage[defineIndex];
                    FE_Assert(desc.m_defines[defineIndex].m_name == define.m_name);
                    FE_Assert(desc.m_defines[defineIndex].m_value == define.m_value);
                }
            }

            return Core::ShaderHandle{ iter->second };
        }

        ShaderInfo* shaderInfo = m_shaderPool.New();
        m_shaders.push_back(shaderInfo);

        shaderInfo->m_name = desc.m_name;
        shaderInfo->m_stage = desc.m_stage;
        shaderInfo->m_entryPoint = GetShaderEntryPointName(desc.m_stage);
        shaderInfo->m_definesStorage.Init(desc.m_defines);
        shaderInfo->m_completionWaitGroup = WaitGroup::Create();

        auto* task = Memory::New<CompilationTask>(&m_taskPool);
        task->m_parent = this;
        task->m_shaderIndex = shaderIndex;
        task->ScheduleBackground(m_jobSystem, shaderInfo->m_completionWaitGroup.Get(), JobPriority::kNormal);

        return Core::ShaderHandle{ shaderIndex };
    }
} // namespace FE::Graphics::Vulkan
