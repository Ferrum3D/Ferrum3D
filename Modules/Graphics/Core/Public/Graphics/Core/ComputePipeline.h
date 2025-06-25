#pragma once
#include <Graphics/Core/PipelineBase.h>

namespace FE::Graphics::Core
{
    struct ComputePipelineDesc final
    {
        Env::Name m_shader;

        [[nodiscard]] uint64_t GetHash() const
        {
            return m_shader.GetHash();
        }

        ComputePipelineDesc& SetComputeShader(const Env::Name shaderName)
        {
            m_shader = shaderName;
            return *this;
        }
    };


    struct ComputePipeline : public PipelineBase
    {
        FE_RTTI_Class(ComputePipeline, "768E754C-D58B-4FDC-BC8D-893E8A7E0438");

        ~ComputePipeline() override = default;

        [[nodiscard]] const ComputePipelineDesc& GetDesc() const
        {
            return m_desc;
        }

    protected:
        ComputePipelineDesc m_desc;
    };
} // namespace FE::Graphics::Core
