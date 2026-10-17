#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Passes/Visibility/Debug.h>
#include <Shaders/Passes/Visibility/DebugCommon.hlsli>

#ifdef SURFACE_GROUP_HEADER
#    include SURFACE_GROUP_HEADER
#endif

[[vk::push_constant]] DebugConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

#ifdef SURFACE_GROUP_HEADER
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel.xy, 0));
    if (id.x == 0)
        return;

    const uint group = SurfaceGroup(GConstants.m_view, id);
    if (group == kInvalidIndex)
        return;
#endif

    const uint g0 = GConstants.m_g0.Load(uint3(pixel.xy, 0));
    const uint route = GBuffer::GetShadingRoute(g0);
    float3 color = kDebugBackgroundColor;
    if (route == 255)
        color = float3(1, 0, 1);
    else if (route != 0)
    {
        const uint3 location = uint3(pixel.xy, 0);
        GBuffer::Packed packed;
        packed.m_g0 = g0;
        packed.m_g1 = GConstants.m_g1.Load(location);
        packed.m_g2 = GConstants.m_g2.Load(location);
        packed.m_g3 = 0;
        if (GBuffer::HasExtendedPayload(packed.m_g2))
            packed.m_g3 = GConstants.m_g3.Load(location);

        const GBuffer::Unpacked surface = GBuffer::Decode(packed);
        if (GConstants.m_mode == DebugMode::kEvaluationTile)
        {
            const uint tilesPerRow = (GConstants.m_resolution.x + kTileSize - 1) / kTileSize;
            const uint tile = pixel.x / kTileSize + pixel.y / kTileSize * tilesPerRow;
            color = DebugIDColor(tile);
        }
        else if (GConstants.m_mode == DebugMode::kEvaluationGroup)
        {
#ifdef SURFACE_GROUP_HEADER
            color = DebugIDColor(group);
#endif
        }
        else if (GConstants.m_mode == DebugMode::kSurfaceColor)
            color = surface.m_color;
        else if (GConstants.m_mode == DebugMode::kSurfaceNormal)
            color = surface.m_normal * 0.5f + 0.5f;
        else if (GConstants.m_mode == DebugMode::kSurfaceProperties)
            color = float3(surface.m_roughness, surface.m_metallic, surface.m_ambientOcclusion);
        else if (GConstants.m_mode == DebugMode::kSurfaceEmissive)
            color = surface.m_emissive / 1000.0f;
        else
            color = DebugIDColor(route);
    }

    GConstants.m_output.Store(pixel.xy, color);
}
