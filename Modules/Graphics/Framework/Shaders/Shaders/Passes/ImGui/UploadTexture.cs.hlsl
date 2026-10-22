#include <Shaders/Passes/ImGui/ImGui.h>

[[vk::push_constant]] TextureUploadConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_size))
        return;

    // Source rows are tightly packed; single-channel textures store coverage in alpha over white.
    const uint32_t pixelIndex = pixel.y * GConstants.m_size.x + pixel.x;
    float4 color;
    if (GConstants.m_bytesPerPixel == 4)
    {
        const uint32_t packed = GConstants.m_source.Load(pixelIndex * 4);
        color = float4(packed & 0xff, (packed >> 8) & 0xff, (packed >> 16) & 0xff, packed >> 24) / 255.0f;
    }
    else
    {
        const uint32_t word = GConstants.m_source.Load((pixelIndex >> 2) * 4);
        const uint32_t alpha = (word >> ((pixelIndex & 3) * 8)) & 0xff;
        color = float4(1.0f, 1.0f, 1.0f, alpha / 255.0f);
    }

    GConstants.m_target.Store(GConstants.m_offset + pixel.xy, color);
}
