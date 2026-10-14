#include <Shaders/Passes/Visibility/Debug.h>
#include <Shaders/Passes/Visibility/SurfaceCodec.hlsli>
[[vk::push_constant]] DebugConstants GConstants;

float3 DebugIDColor(uint id)
{
    id = (id ^ (id >> 16)) * 0x7feb352du;
    id = (id ^ (id >> 15)) * 0x846ca68bu;
    id ^= id >> 16;
    return float3(id & 255, (id >> 8) & 255, (id >> 16) & 255) / 255.0f;
}

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint g0 = GConstants.m_g0.Load(uint3(pixel.xy, 0));
    const uint route = g0 >> 24;
    float3 color = float3(0.02f, 0.02f, 0.04f);
    if (route == 255)
        color = float3(1, 0, 1);
    else if (route != 0)
    {
        if (GConstants.m_mode == 10)
            color = DebugIDColor((pixel.x / kTileSize) + (pixel.y / kTileSize) * ((GConstants.m_resolution.x + 127) / 128));
        else if (GConstants.m_mode == 11)
        {
            const uint instanceIndex = GConstants.m_visibility.Load(uint3(pixel.xy, 0)).x - 1;
            const ViewData view = GConstants.m_view.Load(0);
            const MeshInstanceTable::Row instance = MeshInstanceTable::Create(view.m_instances).ReadRow(instanceIndex);
            color = DebugIDColor(instance.m_meshGroup.Get().m_rowIndex);
        }
        else if (GConstants.m_mode == 12)
            color = DecodeSurfaceColor(g0);
        else if (GConstants.m_mode == 13)
            color = DecodeSurfaceNormal(GConstants.m_g1.Load(uint3(pixel.xy, 0))) * 0.5f + 0.5f;
        else if (GConstants.m_mode == 14)
        {
            const uint properties = GConstants.m_g2.Load(uint3(pixel.xy, 0));
            color = float3(properties & 255, (properties >> 8) & 255, (properties >> 16) & 255) / 255.0f;
        }
        else if (GConstants.m_mode == 15)
        {
            const uint flags = GConstants.m_g2.Load(uint3(pixel.xy, 0)) >> 24;
            color = (flags & kExtendedPayloadValid) != 0 ? DecodeSurfaceEmissive(GConstants.m_g3.Load(uint3(pixel.xy, 0))) / 1000.0f : 0;
        }
        else
            color = DebugIDColor(route);
    }

    GConstants.m_output.Store(pixel.xy, color);
}
