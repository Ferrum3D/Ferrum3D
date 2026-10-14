#include <Shaders/Tests/GpuProbe.h>
#include <Shaders/Passes/Visibility/SurfaceCodec.hlsli>
[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(1, 1, 1)
void main()
{
    const float3 color = float3(0.95f, 0.4f, 0.2f);
    const uint encoded = EncodeSurfaceColor(color, 19);
    const float3 decoded = DecodeSurfaceColor(encoded);
    const float3 n = normalize(float3(-0.2f, 0.7f, -0.5f));
    const float3 restored = DecodeSurfaceNormal(EncodeSurfaceNormal(n));
    const float3 emissive = DecodeSurfaceEmissive(EncodeSurfaceEmissive(float3(1000, 32, 0)));
    GConstants.m_words.Store(0, encoded >> 24);
    GConstants.m_words.Store(1, asuint(max(max(abs(color - decoded).x, abs(color - decoded).y), abs(color - decoded).z)));
    GConstants.m_words.Store(2, asuint(dot(n, restored)));
    GConstants.m_words.Store(3, asuint(emissive.x));
    GConstants.m_words.Store(4, asuint(emissive.y));
    GConstants.m_words.Store(5, asuint(emissive.z));
    GConstants.m_words.Store(6, EncodeSurfaceProperties(0.5f, 1, 0, 1));
    GConstants.m_words.Store(7, EncodeSurfaceColor(float3(0, 1, 0), 0) >> 24);
}
