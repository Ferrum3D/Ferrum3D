#pragma once
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)


    struct DebugConstants final
    {
        ByteAddressBufferDescriptor m_view;
        Texture2DDescriptor<uint2> m_visibility;
        Texture2DDescriptor<float> m_depth;
        Texture2DDescriptor<uint32_t> m_g0;
        Texture2DDescriptor<uint32_t> m_g1;
        Texture2DDescriptor<uint32_t> m_g2;
        Texture2DDescriptor<uint32_t> m_g3;
        RWTexture2DDescriptor<float3> m_output;
        uint2 m_resolution;
        uint32_t m_mode;

        FE_RTTI_Reflect("E4D403A6-9086-46E8-AF82-AB6DAFBEC18A");
    };


    struct DebugPassDesc final
    {
        DebugConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("CE5BD90E-C1D0-4003-864E-473E44D40F0E");
    };

FE_HOST_END_NAMESPACE
