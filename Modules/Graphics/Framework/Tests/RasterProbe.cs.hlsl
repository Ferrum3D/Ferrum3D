#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>
#include <Shaders/Tests/RasterProbe.h>

[[vk::push_constant]] Constants GConstants;


FE_NUM_THREADS(1, 1, 1)
void main(uint3 group : SV_GroupID)
{
    const uint2 pixel = group.x == 0 ? uint2(32, 32) : uint2(0, 0);
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    const float depth = GConstants.m_depth.Load(uint3(pixel, 0));
    ReconstructedAttributes attributes;
    ByteAddressBufferDescriptor objectData;
    objectData.m_index = GConstants.m_view.m_index;
    const MeshViewData view = LoadMeshView(objectData);
    const bool valid = ReconstructAttributes(view, id, float2(pixel) + 0.5f, float2(64, 64), attributes);
    const uint offset = group.x * 16;
    GConstants.m_words.Store(offset, id.x);
    GConstants.m_words.Store(offset + 1, id.y);
    GConstants.m_words.Store(offset + 2, asuint(depth));
    GConstants.m_words.Store(offset + 3, valid ? 1 : 0);
    GConstants.m_words.Store(offset + 4, asuint(attributes.m_uv.x));
    GConstants.m_words.Store(offset + 5, asuint(attributes.m_uv.y));
    GConstants.m_words.Store(offset + 6, asuint(attributes.m_normal.z));
    GConstants.m_words.Store(offset + 7, asuint(attributes.m_tangent.x));
    GConstants.m_words.Store(offset + 8, asuint(attributes.m_tangent.w));
    GConstants.m_words.Store(offset + 9, asuint(attributes.m_uvDdx.x));
    GConstants.m_words.Store(offset + 10, asuint(attributes.m_uvDdy.y));
    GConstants.m_words.Store(offset + 11, asuint(attributes.m_depth));
    const uint color = GConstants.m_g0.Load(uint3(pixel, 0));
    GConstants.m_words.Store(offset + 12, color >> 24);
    GConstants.m_words.Store(offset + 13, color);
    GBuffer::Packed packed = (GBuffer::Packed)0;
    packed.m_g1 = GConstants.m_g1.Load(uint3(pixel, 0));
    const GBuffer::Unpacked surface = GBuffer::Decode(packed);
    const float normalZ = color >> 24 != 0 ? surface.m_normal.z : 0;
    GConstants.m_words.Store(offset + 14, asuint(normalZ));
    GConstants.m_words.Store(offset + 15, color >> 24 != 0 ? GConstants.m_g2.Load(uint3(pixel, 0)) : 0);
}
