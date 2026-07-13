#include <Core/IO/Async.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/ShaderReflection.h>

namespace FE::Graphics::Vulkan
{
    ShaderLibrary::ShaderLibrary(Core::Device* device, Core::ShaderCompiler* shaderCompiler)
        : m_shaderPool("Graphics/ShaderLibrary/ShaderInfoPool", sizeof(ShaderInfo))
        , m_shaderCompiler(shaderCompiler)
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

        Jobs::DispatchBackground(
            [this, shaderIndex] {
                CompileShader(shaderIndex);
            },
            shaderInfo->m_completionWaitGroup.Get());

        return Core::ShaderHandle{ shaderIndex };
    }


    void ShaderLibrary::CompileShader(const uint32_t shaderIndex)
    {
        std::unique_lock lock{ m_lock };

        ShaderInfo* shaderInfo = m_shaders[shaderIndex];

        lock.unlock();

        FE_PROFILER_ZONE_TEXT(shaderInfo->m_name.c_str());

        Memory::FiberTempAllocator temp;
        const auto definesList = Core::SplitDefines(shaderInfo->m_defines, &temp);

        const IO::PathView pathView{ shaderInfo->m_name };

        Core::ShaderCompilerArgs args;
        args.m_shaderName = shaderInfo->m_name;
        args.m_stage = Core::GetShaderStageFromName(pathView.stem());
        args.m_defines = definesList;

        const Core::ShaderCompilerResult result = m_shaderCompiler->CompileShader(args);
        if (!result.m_codeValid)
            return;

        FE_AssertDebug(IsAlignedPtr(result.m_byteCode.data(), sizeof(uint32_t)));

        const VkDevice device = NativeCast(m_device);

        VkShaderModuleCreateInfo shaderModuleCI{};
        shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCI.codeSize = result.m_byteCodeSize;
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(result.m_byteCode.data());
        VerifyVk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderInfo->m_shaderModule));

        const festd::span byteCode{ shaderModuleCI.pCode,
                                    shaderModuleCI.pCode + Math::CeilDivide(shaderModuleCI.codeSize, sizeof(uint32_t)) };
        shaderInfo->m_reflection = Memory::DefaultNew<ShaderReflection>(byteCode);
    }
} // namespace FE::Graphics::Vulkan
