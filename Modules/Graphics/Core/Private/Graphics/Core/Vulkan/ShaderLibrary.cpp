#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/FiberTempAllocator.h>
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
        std::unique_lock lock{ m_parent->m_lock };

        ShaderInfo* shaderInfo = m_parent->m_shaders[m_shaderIndex];

        lock.unlock();

        FE_PROFILER_ZONE_TEXT(shaderInfo->m_name.c_str());

        auto defer = festd::defer([this] {
            Memory::Delete(&m_parent->m_taskPool, this, sizeof(*this));
        });

        Core::ShaderCompiler* compiler = m_parent->m_shaderCompiler;

        Memory::FiberTempAllocator temp;
        const auto definesList = Core::SplitDefines(shaderInfo->m_defines, &temp);

        const IO::PathView pathView{ shaderInfo->m_name };

        Core::ShaderCompilerArgs args;
        args.m_shaderName = shaderInfo->m_name;
        args.m_stage = Core::GetShaderStageFromName(pathView.stem());
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
        shaderInfo->m_reflection = Rc<ShaderReflection>::New(&m_parent->m_reflectionPool, byteCode);
    }


    ShaderLibrary::ShaderLibrary(Core::Device* device, Core::ShaderCompiler* shaderCompiler, IJobSystem* jobSystem)
        : m_taskPool("Graphics/Core/ShaderLibrary/TaskPool", sizeof(CompilationTask))
        , m_shaderPool("Graphics/Core/ShaderLibrary/ShaderInfoPool", sizeof(ShaderInfo))
        , m_reflectionPool("Graphics/Core/ShaderLibrary/ShaderReflectionPool", sizeof(ShaderReflection))
        , m_shaderCompiler(shaderCompiler)
        , m_jobSystem(jobSystem)
    {
        m_device = device;
    }


    ShaderLibrary::~ShaderLibrary()
    {
        for (ShaderInfo* shaderInfo : m_shaders)
        {
            const VkDevice device = NativeCast(m_device);
            vkDestroyShaderModule(device, shaderInfo->m_shaderModule, nullptr);
            Memory::Delete(&m_shaderPool, shaderInfo, sizeof(*shaderInfo));
        }
    }


    Core::ShaderHandle ShaderLibrary::GetShader(const Env::Name name, const Env::Name defines)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_lock };

        const uint64_t hash = HashAll(name, defines);
        const uint32_t shaderIndex = m_shaders.size();
        const auto [iter, inserted] = m_shadersMap.insert({ hash, shaderIndex });
        if (!inserted)
        {
            if (Build::IsDebug())
            {
                const ShaderInfo* shaderInfo = m_shaders[iter->second];
                FE_Assert(shaderInfo->m_name == name);
                FE_Assert(shaderInfo->m_defines == defines);
            }

            return Core::ShaderHandle{ iter->second };
        }

        auto* shaderInfo = Memory::New<ShaderInfo>(&m_shaderPool);
        m_shaders.push_back(shaderInfo);

        shaderInfo->m_name = name;
        shaderInfo->m_defines = defines;
        shaderInfo->m_entryPoint = "main";
        shaderInfo->m_completionWaitGroup = WaitGroup::Create();

        auto* task = Memory::New<CompilationTask>(&m_taskPool);
        task->m_parent = this;
        task->m_shaderIndex = shaderIndex;
        task->ScheduleBackground(m_jobSystem, shaderInfo->m_completionWaitGroup.Get(), JobPriority::kNormal);

        return Core::ShaderHandle{ shaderIndex };
    }
} // namespace FE::Graphics::Vulkan
