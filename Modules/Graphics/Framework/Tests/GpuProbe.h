#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Stage1Probe)

    struct Constants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_words;
        uint32_t m_writeArguments;
        BufferPointer m_material;
        BufferPointer m_instance;

        FE_RTTI_Reflect("83EEEF7D-FAF7-4AA2-BC79-642BF7D1002A");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("1B049A95-2315-48E4-9039-87D85925DD26");
    };


    struct IndirectPassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;

        FE_RTTI_Reflect("F41CD80E-21DB-4956-AB77-024A73D425D3");
    };

FE_HOST_END_NAMESPACE
