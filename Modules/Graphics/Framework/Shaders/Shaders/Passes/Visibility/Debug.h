#pragma once
#include <Shaders/Passes/Visibility/DebugModes.h>
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)


    struct DebugConstants final
    {
        AnyConstantsPtr m_view;
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

FE_HOST_END_NAMESPACE
