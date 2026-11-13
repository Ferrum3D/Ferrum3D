#pragma once
#include <Shaders/Scene/ViewData.h>

namespace ScreenTrace
{
    //! The camera depth that rays are traced through. Invalid in passes whose surfaces are absent from the camera depth, such
    //! as forward transparency.
    struct Context
    {
        Texture2DDescriptor<float> m_depth;
        ViewData m_view;
        bool m_isValid;
    };


    Context CreateContext(const Texture2DDescriptor<float> depth, const ViewData view)
    {
        Context context;
        context.m_depth = depth;
        context.m_view = view;
        context.m_isValid = true;
        return context;
    }


    Context None()
    {
        return (Context)0;
    }


    //! Whether the camera depth shows an occluder on the segment of rayLength from the origin along the direction. A step is
    //! occluded when it lies behind the visible surface by more than the bias and less than the ray length, which is the
    //! thickness that surfaces are assumed to have. Steps that leave the screen end the trace as unoccluded.
    bool IsOccluded(const Context context, const float3 origin, const float3 direction, const float rayLength, const float bias,
                    const uint stepCount)
    {
        const ViewData view = context.m_view;
        Texture2D<float> depthTexture = context.m_depth.Get();
        for (uint step = 0; step < stepCount; ++step)
        {
            const float3 position = origin + direction * (rayLength * float(step + 1) / float(stepCount));
            const float4 clip = mul(float4(position, 1.0f), view.m_viewProjection);
            if (clip.w <= 0.0f)
                return false;

            const float2 ndc = clip.xy / clip.w;
            if (any(abs(ndc) >= 1.0f))
                return false;

            // Reverse Z: zero is the background, which occludes nothing.
            const float2 pixel = (ndc * float2(0.5f, -0.5f) + 0.5f) * view.m_viewport.zw + view.m_viewport.xy;
            const float sceneDepth = depthTexture.Load(int3(pixel, 0));
            if (sceneDepth <= 0.0f)
                continue;

            // Both distances are measured from the camera along the same camera ray.
            const float4 sceneWorld = mul(float4(ndc, sceneDepth, 1.0f), view.m_inverseViewProjection);
            const float sceneDistance = length(sceneWorld.xyz / sceneWorld.w - view.m_cameraPosition.xyz);
            const float behind = length(position - view.m_cameraPosition.xyz) - sceneDistance;
            if (behind > bias && behind < rayLength)
                return true;
        }

        return false;
    }
} // namespace ScreenTrace
