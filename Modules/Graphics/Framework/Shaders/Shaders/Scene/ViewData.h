#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    struct ViewData final
    {
        float4x4 m_viewProjection;
        float4x4 m_inverseViewProjection;
        float4x4 m_unjitteredViewProjection;
        float4x4 m_previousViewProjection;
        float4 m_cameraPosition;
        float4 m_viewport;
        float4 m_backgroundColor;
        //! xy: projection jitter in render pixels; z: material texture LOD bias.
        float4 m_jitterAndLodBias;
        FE_RTTI_Reflect("D0EDE1DA-0E96-4FCB-BEC7-F0448213B493");
    };
#if !FE_HOST
    // Camera constants are shared by the entire dispatch, including partially active waves.
    ViewData WaveUniformView(ViewData view)
    {
        [unroll] for (uint row = 0; row < 4; ++row)
        {
            view.m_viewProjection[row] = WaveReadLaneFirst(view.m_viewProjection[row]);
            view.m_inverseViewProjection[row] = WaveReadLaneFirst(view.m_inverseViewProjection[row]);
            view.m_unjitteredViewProjection[row] = WaveReadLaneFirst(view.m_unjitteredViewProjection[row]);
            view.m_previousViewProjection[row] = WaveReadLaneFirst(view.m_previousViewProjection[row]);
        }

        view.m_cameraPosition = WaveReadLaneFirst(view.m_cameraPosition);
        view.m_viewport = WaveReadLaneFirst(view.m_viewport);
        view.m_backgroundColor = WaveReadLaneFirst(view.m_backgroundColor);
        view.m_jitterAndLodBias = WaveReadLaneFirst(view.m_jitterAndLodBias);
        return view;
    }
#endif
FE_HOST_END_NAMESPACE
