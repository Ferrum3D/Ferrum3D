#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::ConstantsProbe)

    struct Inner final
    {
        RWStructuredBufferDescriptor<uint32_t> m_output;
        uint32_t m_value;

        FE_RTTI_Reflect("A47DA744-16FD-4B6D-AF7A-22BAE58CCBDB");
    };


    struct Outer final
    {
        ConstantsPtr<Inner> m_inner;
        uint32_t m_multiplier;

        FE_RTTI_Reflect("5E0E7D60-2E7B-4B7C-9F5A-6C0B8D3A1F42");
    };


    //! The output buffer is declared only through these pointers, so the shader can write it only if the graph finds it.
    struct Constants final
    {
        ConstantsPtr<Outer> m_outer;
        AnyConstantsPtr m_erased;

        FE_RTTI_Reflect("0C7E4B19-93D2-4F6E-8A71-D25F3E6B9C08");
    };

FE_HOST_END_NAMESPACE
