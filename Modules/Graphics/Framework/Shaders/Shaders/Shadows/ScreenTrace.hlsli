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
        //! Clip z of a perspective camera is x * w + y, where w is the distance along the view direction. Zero for other
        //! cameras, which trace nothing.
        float2 m_clipDepth;
        bool m_isPerspective;
        bool m_isValid;
    };


    Context CreateContext(const Texture2DDescriptor<float> depth, const ViewData view)
    {
        // The projection maps the distance along the view direction to both z and w, so the z column of the matrix is a
        // multiple of the direction in its w column plus a constant.
        const float4x4 viewProjection = view.m_viewProjection;
        const float3 depthAxis = float3(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2]);
        const float3 viewAxis = float3(viewProjection[0][3], viewProjection[1][3], viewProjection[2][3]);
        const float viewAxisLengthSquared = dot(viewAxis, viewAxis);

        Context context;
        context.m_depth = depth;
        context.m_view = view;
        context.m_isPerspective = viewAxisLengthSquared > 0.0f;
        context.m_clipDepth = 0.0f;
        if (context.m_isPerspective)
        {
            context.m_clipDepth.x = dot(depthAxis, viewAxis) / viewAxisLengthSquared;
            context.m_clipDepth.y = viewProjection[3][2] - context.m_clipDepth.x * viewProjection[3][3];
        }

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
    //!
    //! Clip coordinates are linear in world space, so the ray is projected once and its steps interpolate between the ends.
    //! The visible surface is on the camera ray of its step, so their distances from the camera relate as their w.
    bool IsOccluded(const Context context, const float3 origin, const float3 direction, const float rayLength, const float bias,
                    const uint stepCount)
    {
        if (!context.m_isPerspective)
            return false;

        const ViewData view = context.m_view;
        const float3 rayEnd = origin + direction * rayLength;
        const float3 originClip = mul(float4(origin, 1.0f), view.m_viewProjection).xyw;
        const float3 endClip = mul(float4(rayEnd, 1.0f), view.m_viewProjection).xyw;
        const float3 originOffset = origin - view.m_cameraPosition.xyz;

        Texture2D<float> depthTexture = context.m_depth.Get();
        for (uint step = 0; step < stepCount; ++step)
        {
            const float fraction = float(step + 1) / float(stepCount);
            const float3 clip = lerp(originClip, endClip, fraction);
            if (clip.z <= 0.0f)
                return false;

            const float2 ndc = clip.xy / clip.z;
            if (any(abs(ndc) >= 1.0f))
                return false;

            // Reverse Z: zero is the background, which occludes nothing.
            const float2 pixel = (ndc * float2(0.5f, -0.5f) + 0.5f) * view.m_viewport.zw + view.m_viewport.xy;
            const float sceneDepth = depthTexture.Load(int3(pixel, 0));
            if (sceneDepth <= 0.0f)
                continue;

            const float sceneW = context.m_clipDepth.y / (sceneDepth - context.m_clipDepth.x);
            const float stepDistance = length(originOffset + direction * (rayLength * fraction));
            const float behind = stepDistance * (1.0f - sceneW / clip.z);
            if (behind > bias && behind < rayLength)
                return true;
        }

        return false;
    }
} // namespace ScreenTrace
