#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Tests/GpuProbe.h>
[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(1, 1, 1)
void main()
{
    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = float3(0.95f, 0.4f, 0.2f);
    surface.m_normal = normalize(float3(-0.2f, 0.7f, -0.5f));
    surface.m_emissive = float3(1000, 32, 0);
    surface.m_roughness = 0.5f;
    surface.m_metallic = 1;
    surface.m_shadingRoute = 19;
    const GBuffer::Packed packed = GBuffer::Encode(surface);
    const GBuffer::Unpacked restored = GBuffer::Decode(packed);
    const float3 colorError = abs(surface.m_color - restored.m_color);
    const float maximumColorError = Math::Max3(colorError.x, colorError.y, colorError.z);
    const float normalSimilarity = dot(surface.m_normal, restored.m_normal);
    GConstants.m_words.Store(0, restored.m_shadingRoute);
    GConstants.m_words.Store(1, asuint(maximumColorError));
    GConstants.m_words.Store(2, asuint(normalSimilarity));
    GConstants.m_words.Store(3, asuint(restored.m_emissive.x));
    GConstants.m_words.Store(4, asuint(restored.m_emissive.y));
    GConstants.m_words.Store(5, asuint(restored.m_emissive.z));
    GConstants.m_words.Store(6, packed.m_g2);
    surface.m_shadingRoute = 0;
    const GBuffer::Packed background = GBuffer::Encode(surface);
    GConstants.m_words.Store(7, background.m_g0 >> 24);
}
