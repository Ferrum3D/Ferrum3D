#pragma once
#include <Graphics/Core/ComputePipeline.h>
#include <Graphics/Core/GraphicsPipeline.h>
#include <Graphics/Core/ShaderLibrary.h>

namespace FE::Graphics::Core
{
    enum class PipelinePriority : uint32_t
    {
        kNormal,
        kHigh,
        kHighest,
        kCount,
    };


    struct PipelineRequestBase
    {
        PipelinePriority m_priority = PipelinePriority::kNormal;
        Env::Name m_defines;
        festd::span<const ShaderSpecializationConstant> m_specializationConstants;
    };


    struct GraphicsPipelineRequest final : public PipelineRequestBase
    {
        GraphicsPipelineDesc m_desc;

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.UpdateRaw(m_desc.GetHash());
            hasher.UpdateRaw(m_defines.GetHash());

            for (const ShaderSpecializationConstant constant : m_specializationConstants)
                hasher.UpdateRaw(constant.GetHash());

            return hasher.Finalize();
        }
    };


    struct ComputePipelineRequest final : public PipelineRequestBase
    {
        ComputePipelineDesc m_desc;

        [[nodiscard]] uint64_t GetHash() const
        {
            Hasher hasher;
            hasher.UpdateRaw(m_desc.GetHash());
            hasher.UpdateRaw(m_defines.GetHash());

            for (const ShaderSpecializationConstant constant : m_specializationConstants)
                hasher.UpdateRaw(constant.GetHash());

            return hasher.Finalize();
        }
    };


    struct PipelineFactory : public DeviceObject
    {
        ~PipelineFactory() override = default;

        FE_RTTI_Class(PipelineFactory, "CD16508A-5F34-4700-8D23-AF217B55FFD1");

        virtual GraphicsPipeline* CreateGraphicsPipeline(const GraphicsPipelineRequest& request) = 0;
        virtual ComputePipeline* CreateComputePipeline(const ComputePipelineRequest& request) = 0;
    };
} // namespace FE::Graphics::Core
