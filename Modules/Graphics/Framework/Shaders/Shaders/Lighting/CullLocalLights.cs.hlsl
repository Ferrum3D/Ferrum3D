#include <Shaders/Lighting/LocalLightTable.hlsli>

[[vk::push_constant]] CullLocalLightsConstants GConstants;

float3 ClusterCorner(LightClusterData data, uint2 tile, float z)
{
    const float2 pixel = min(float2(tile * kLightClusterTileSize), data.m_viewport.zw);
    const float2 ndc = pixel / data.m_viewport.zw * float2(2, -2) + float2(-1, 1);
    const float4 projected = mul(float4(0, 0, z, 1), data.m_projection);
    const float4 world = mul(float4(ndc, projected.z / projected.w, 1), data.m_inverseViewProjection);
    return world.xyz / world.w;
}


FE_NUM_THREADS(32, 1, 1)
void main(uint3 cluster : SV_GroupID, uint lane : SV_GroupIndex)
{
    const LocalLightingBinding lighting = GConstants.m_lighting;
    const LightClusterData data = lighting.m_clusters.Load(0);
    const float z0 = data.m_depth.x * exp2(cluster.z / data.m_depth.z);
    const float z1 = data.m_depth.x * exp2((cluster.z + 1) / data.m_depth.z);
    float3 lower = (float3)1e30;
    float3 upper = (float3)-1e30;
    [unroll] for (uint corner = 0; corner < 8; ++corner)
    {
        const float3 p = ClusterCorner(data, cluster.xy + uint2(corner & 1, (corner >> 1) & 1), (corner & 4) != 0 ? z1 : z0);
        lower = min(lower, p);
        upper = max(upper, p);
    }

    const float3 center = (lower + upper) * 0.5f;
    const float radius = length(upper - lower) * 0.5f;
    const uint base = ((cluster.z * data.m_tiles.y + cluster.y) * data.m_tiles.x + cluster.x) * data.m_wordCount;
    // Each lane owns a whole mask word, so there is no append limit or atomic contention.
    for (uint word = lane; word < data.m_wordCount; word += 32)
    {
        uint mask = 0;
        for (uint bit = 0; bit < 32; ++bit)
        {
            const uint index = word * 32 + bit;
            if (index >= data.m_lightCount)
                break;

            const DecodedLocalLight light = LoadLocalLight(lighting, index);
            if (light.m_positionRange.w <= 0)
                continue;

            const float3 delta = light.m_positionRange.xyz - clamp(light.m_positionRange.xyz, lower, upper);
            bool intersects = dot(delta, delta) <= light.m_positionRange.w * light.m_positionRange.w;
            if (intersects && light.m_directionOuterCos.w > -1)
            {
                // Conservative cone / cluster bounding sphere test, including the apex and rear plane.
                const float3 v = center - light.m_positionRange.xyz;
                const float axial = dot(v, light.m_directionOuterCos.xyz);
                const float radial = sqrt(max(0.0f, dot(v, v) - axial * axial));
                const float cosine = light.m_directionOuterCos.w;
                const float sine = sqrt(saturate(1 - cosine * cosine));
                intersects = axial >= -radius && cosine * radial - sine * axial <= radius;
            }

            if (intersects)
                mask |= 1u << bit;
        }

        GConstants.m_masks.Store(base + word, mask);
    }
}
