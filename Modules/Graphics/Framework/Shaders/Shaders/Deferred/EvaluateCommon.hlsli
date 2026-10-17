#pragma once
#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Deferred/PixelWork.hlsli>
#include <Shaders/Deferred/Surface.h>

[[vk::push_constant]] SurfaceConstants GConstants;

bool LoadEvaluationPixel(uint3 dispatch, uint lane, out uint2 pixel, out uint groupIndex)
{
    const ShaderRange range = LoadSurfaceRange(GConstants.m_ranges, GConstants.m_shaderSlot);
    PixelWork work;
    const bool valid = LoadPixelWork(range,
                                     GConstants.m_waves,
                                     GConstants.m_pixels,
                                     GConstants.m_resolution,
                                     GConstants.m_tilesPerRow,
                                     dispatch.x * kSurfaceWavesPerGroup + lane / EVALUATION_LANES,
                                     lane % EVALUATION_LANES,
                                     work);
    pixel = work.m_pixel;
    groupIndex = work.m_command.m_group;
    return valid;
}


void StoreGBuffer(const uint2 pixel, const GBuffer::Unpacked surface)
{
    const GBuffer::Packed packed = GBuffer::Encode(surface);
    GConstants.m_g0.Store(pixel, packed.m_g0);
    GConstants.m_g1.Store(pixel, packed.m_g1);
    GConstants.m_g2.Store(pixel, packed.m_g2);
    if (GBuffer::HasExtendedPayload(packed.m_g2))
        GConstants.m_g3.Store(pixel, packed.m_g3);
}
