#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/RHI/ShaderCompiler.h>
#include <Graphics/RHI/ShaderLibrary.h>
#include <Graphics/RHI/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct ShaderReflection;

    struct ShaderLibrary final : public RHI::ShaderLibrary
    {
        ShaderLibrary(RHI::ShaderCompiler* shaderCompiler, Logger* logger, IO::IAsyncStreamIO* asyncIO);

        RHI::ShaderHandle GetShader(const RHI::ShaderPermutationDesc& desc) override;

    private:
        struct TaskImplementation;

        struct ShaderInfo final
        {
            RHI::ShaderPermutationDesc m_permutationDesc;
            VkShaderModule m_shaderModule = VK_NULL_HANDLE;
            Rc<ShaderReflection> m_reflection;
            Env::Name m_entryPoint;
            std::atomic<bool> m_loaded;
        };

        Memory::PoolAllocator m_taskPool;
        Memory::LinearAllocator m_shaderInfoAllocator;
        festd::unordered_dense_map<uint64_t, uint32_t> m_shadersMap;
        SegmentedVector<ShaderInfo> m_shaders;
        RHI::ShaderCompiler* m_shaderCompiler;
        Logger* m_logger;
        IO::IAsyncStreamIO* m_asyncIO;
    };
} // namespace FE::Graphics::Vulkan
