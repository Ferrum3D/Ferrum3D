#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Common/ShaderSourceCache.h>
#include <Graphics/Core/ShaderCompiler.h>
#include <Graphics/Core/ShaderLibrary.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderReflection;

    struct ShaderModuleInfo final
    {
        VkShaderModule m_shaderModule = VK_NULL_HANDLE;
        const char* m_entryPoint = nullptr;
    };

    struct ShaderLibrary final : public Core::ShaderLibrary
    {
        FE_RTTI_Class(ShaderLibrary, "E2254CBD-679C-4310-87CF-FA8DA780BDA1");

        ShaderLibrary(Core::Device* device, Core::ShaderCompiler* shaderCompiler, IJobSystem* jobSystem);
        ~ShaderLibrary() override;

        Core::ShaderHandle GetShader(const Core::ShaderPermutationDesc& desc) override;

        [[nodiscard]] WaitGroup* GetCompletionWaitGroup(const Core::ShaderHandle shaderHandle) const
        {
            return m_shaders[shaderHandle.m_value]->m_completionWaitGroup.Get();
        }

        [[nodiscard]] ShaderModuleInfo GetShaderModule(const Core::ShaderHandle shaderHandle) const
        {
            const ShaderInfo* shaderInfo = m_shaders[shaderHandle.m_value];
            ShaderModuleInfo shaderModuleInfo;
            shaderModuleInfo.m_shaderModule = shaderInfo->m_shaderModule;
            shaderModuleInfo.m_entryPoint = shaderInfo->m_entryPoint;
            return shaderModuleInfo;
        }

        [[nodiscard]] ShaderReflection* GetReflection(const Core::ShaderHandle shaderHandle) const
        {
            return m_shaders[shaderHandle.m_value]->m_reflection.Get();
        }

    private:
        struct CompilationTask;

        struct ShaderInfo final
        {
            Core::DefinesStorage m_definesStorage;
            Env::Name m_name;
            Core::ShaderStage m_stage = Core::ShaderStage::kUndefined;
            VkShaderModule m_shaderModule = VK_NULL_HANDLE;
            Rc<ShaderReflection> m_reflection;
            Rc<WaitGroup> m_completionWaitGroup;
            const char* m_entryPoint = nullptr;
        };

        Memory::PoolAllocator m_taskPool;
        Memory::Pool<ShaderInfo> m_shaderPool;
        Memory::Pool<ShaderReflection> m_reflectionPool;
        festd::unordered_dense_map<uint64_t, uint32_t> m_shadersMap;
        SegmentedVector<ShaderInfo*> m_shaders;
        Core::ShaderCompiler* m_shaderCompiler = nullptr;
        IJobSystem* m_jobSystem = nullptr;
    };
} // namespace FE::Graphics::Vulkan
