#include <Core/IO/Async.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    ShaderVersion::ShaderVersion(const VkDevice device, const VkShaderModule shaderModule, ShaderReflection* reflection)
        : m_device(device)
        , m_shaderModule(shaderModule)
        , m_reflection(reflection)
    {
    }


    ShaderVersion::~ShaderVersion()
    {
        vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    }


    ShaderLibrary::ShaderLibrary(Core::Device* device, Core::ShaderCompiler* shaderCompiler)
        : m_shaderPool("Graphics/ShaderLibrary/ShaderInfoPool", sizeof(ShaderInfo))
        , m_shaderCompiler(shaderCompiler)
    {
        m_device = device;
    }


    ShaderLibrary::~ShaderLibrary()
    {
        for (ShaderInfo* shaderInfo : m_shaders)
            Memory::Delete(&m_shaderPool, shaderInfo, sizeof(*shaderInfo));
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
        shaderInfo->m_completionWaitGroup = WaitGroup::Create();

        Jobs::DispatchBackground(
            [this, shaderIndex] {
                CompileShader(shaderIndex);
            },
            shaderInfo->m_completionWaitGroup.Get());

        return Core::ShaderHandle{ shaderIndex };
    }


    Rc<Core::ShaderReflection> ShaderLibrary::GetCompiledReflection(const Env::Name name, const Env::Name defines)
    {
        const Core::ShaderHandle shader = GetShader(name, defines);
        GetCompletionWaitGroup(shader)->Wait();

        const Rc version = GetVersion(shader);
        if (!version)
            return nullptr;

        return version->GetReflection();
    }


    Core::ShaderHandle ShaderLibrary::FindShader(const Env::Name name, const Env::Name defines)
    {
        std::lock_guard lock{ m_lock };

        const auto iter = m_shadersMap.find(HashAll(name, defines));
        if (iter == m_shadersMap.end())
            return Core::ShaderHandle{};

        return Core::ShaderHandle{ iter->second };
    }


    WaitGroup* ShaderLibrary::GetCompletionWaitGroup(const Core::ShaderHandle shaderHandle) const
    {
        return m_shaders[shaderHandle.m_value]->m_completionWaitGroup.Get();
    }


    Rc<ShaderVersion> ShaderLibrary::GetVersion(const Core::ShaderHandle shaderHandle)
    {
        const ShaderInfo* shaderInfo = m_shaders[shaderHandle.m_value];
        FE_Assert(shaderInfo->m_completionWaitGroup->IsSignaled());

        std::lock_guard lock{ m_lock };
        return shaderInfo->m_version;
    }


    void ShaderLibrary::FindDependentShaders(const festd::span<const Env::Name> paths, festd::vector<Core::ShaderHandle>& shaders)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_lock };

        for (uint32_t shaderIndex = 0; shaderIndex < m_shaders.size(); ++shaderIndex)
        {
            const ShaderInfo* shaderInfo = m_shaders[shaderIndex];
            if (!shaderInfo->m_completionWaitGroup->IsSignaled())
                continue;

            for (const Env::Name dependency : shaderInfo->m_dependencies)
            {
                if (festd::find_index(paths, dependency) != kInvalidIndex)
                {
                    shaders.push_back(Core::ShaderHandle{ shaderIndex });
                    break;
                }
            }
        }
    }


    bool ShaderLibrary::Recompile(const Core::ShaderHandle shaderHandle)
    {
        ShaderInfo* shaderInfo = m_shaders[shaderHandle.m_value];
        FE_Assert(shaderInfo->m_completionWaitGroup->IsSignaled());

        festd::inline_vector<Env::Name, 16> dependencies;
        Rc version = CompileVersion(*shaderInfo, dependencies);

        // A failed compilation still records its dependencies, so that fixing any of them triggers another attempt.
        std::lock_guard lock{ m_lock };
        shaderInfo->m_dependencies = std::move(dependencies);
        if (!version)
            return false;

        shaderInfo->m_version = std::move(version);
        return true;
    }


    void ShaderLibrary::CompileShader(const uint32_t shaderIndex)
    {
        std::unique_lock lock{ m_lock };
        ShaderInfo* shaderInfo = m_shaders[shaderIndex];
        lock.unlock();

        festd::inline_vector<Env::Name, 16> dependencies;
        Rc version = CompileVersion(*shaderInfo, dependencies);

        lock.lock();
        shaderInfo->m_dependencies = std::move(dependencies);
        shaderInfo->m_version = std::move(version);
    }


    Rc<ShaderVersion> ShaderLibrary::CompileVersion(const ShaderInfo& shaderInfo,
                                                    festd::inline_vector<Env::Name, 16>& dependencies)
    {
        FE_PROFILER_ZONE_TEXT(shaderInfo.m_name.c_str());

        Memory::FiberTempAllocator temp;
        const auto definesList = Core::SplitDefines(shaderInfo.m_defines, &temp);

        const IO::PathView pathView{ shaderInfo.m_name };

        Core::ShaderCompilerArgs args;
        args.m_shaderName = shaderInfo.m_name;
        args.m_stage = Core::GetShaderStageFromName(pathView.stem());
        args.m_defines = definesList;

        Core::ShaderCompilerResult result = m_shaderCompiler->CompileShader(args);
        dependencies = std::move(result.m_dependencies);
        if (!result.m_codeValid)
            return nullptr;

        FE_AssertDebug(IsAlignedPtr(result.m_byteCode.data(), sizeof(uint32_t)));

        const VkDevice device = NativeCast(m_device);

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = result.m_byteCodeSize;
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(result.m_byteCode.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VerifyVk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

        const festd::span byteCode{ shaderModuleCI.pCode,
                                    shaderModuleCI.pCode + Math::CeilDivide(shaderModuleCI.codeSize, sizeof(uint32_t)) };
        ShaderReflection* reflection = Memory::DefaultNew<ShaderReflection>(byteCode);
        return Memory::DefaultNew<ShaderVersion>(device, shaderModule, reflection);
    }
} // namespace FE::Graphics::Vulkan
