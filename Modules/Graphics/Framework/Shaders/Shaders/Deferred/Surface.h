#pragma once
#include <Shaders/Deferred/Tiling.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    struct SurfaceConstants final
    {
        AnyConstantsPtr m_view;
        Texture2DDescriptor<uint2> m_visibility;
        SurfaceWorkBinding m_work;
        RWTexture2DDescriptor<uint32_t> m_g0;
        RWTexture2DDescriptor<uint32_t> m_g1;
        RWTexture2DDescriptor<uint32_t> m_g2;
        RWTexture2DDescriptor<uint32_t> m_g3;
        FE_RTTI_Reflect("7BE23B88-FFAC-4420-95C8-7593DB85EFF2");
    };

FE_HOST_END_NAMESPACE
