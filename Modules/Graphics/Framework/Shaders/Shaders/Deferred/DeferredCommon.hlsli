#pragma once
#include <Shaders/Deferred/Deferred.h>
#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Deferred/PixelWork.hlsli>

#ifndef EVALUATION_LANES
#    define EVALUATION_LANES 32
#endif

[[vk::push_constant]] DeferredConstants GConstants;

bool LoadShadingPixel(uint3 dispatch, uint lane, out uint2 pixel)
{
    const ShaderRange range = GConstants.m_ranges.Load(GConstants.m_shaderSlot);
    PixelWork work;
    const bool valid = LoadPixelWork(range,
                                     GConstants.m_waves,
                                     GConstants.m_pixels,
                                     GConstants.m_resolution,
                                     GConstants.m_tilesPerRow,
                                     dispatch.x,
                                     lane,
                                     work);
    pixel = work.m_pixel;
    return valid;
}


GBuffer::Unpacked LoadGBuffer(const uint2 pixel)
{
    const uint3 location = uint3(pixel, 0);
    GBuffer::Packed packed;
    packed.m_g0 = GConstants.m_g0.Load(location);
    packed.m_g1 = GConstants.m_g1.Load(location);
    packed.m_g2 = GConstants.m_g2.Load(location);
    packed.m_g3 = 0;
    if (GBuffer::HasExtendedPayload(packed.m_g2))
        packed.m_g3 = GConstants.m_g3.Load(location);

    return GBuffer::Decode(packed);
}
