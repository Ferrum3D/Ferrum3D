#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Fsr3Probe)
    struct FillConstants final
    {
        RWTexture2DDescriptor<float4> m_color;
        RWTexture2DDescriptor<float> m_depth;
        RWTexture2DDescriptor<float2> m_motion;
        RWTexture2DDescriptor<float> m_reactive;
        uint2 m_size;
        float4 m_value;
        FE_RTTI_Reflect("C06D6577-6597-4F18-B05C-025B1B8889E1");
    };


    struct InspectConstants final
    {
        Texture2DDescriptor<float4> m_color;
        RWStructuredBufferDescriptor<float4> m_pixels;
        uint2 m_size;
        FE_RTTI_Reflect("F73C3073-1C65-4E96-8283-A66F081EB965");
    };
FE_HOST_END_NAMESPACE
