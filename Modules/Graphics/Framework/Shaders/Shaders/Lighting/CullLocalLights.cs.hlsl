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


bool IntersectsSphere(const float4 positionRange, const float3 lower, const float3 upper)
{
    const float3 delta = positionRange.xyz - clamp(positionRange.xyz, lower, upper);
    return positionRange.w > 0 && dot(delta, delta) <= positionRange.w * positionRange.w;
}


// Conservative cone / cluster bounding sphere test, including the apex and rear plane.
bool IntersectsCone(const DecodedLocalLight light, const float3 center, const float radius)
{
    if (light.m_directionOuterCos.w <= -1)
        return true;

    const float3 v = center - light.m_positionRange.xyz;
    const float axial = dot(v, light.m_directionOuterCos.xyz);
    const float radial = sqrt(max(0.0f, dot(v, v) - axial * axial));
    const float cosine = light.m_directionOuterCos.w;
    const float sine = sqrt(saturate(1 - cosine * cosine));
    return axial >= -radius && cosine * radial - sine * axial <= radius;
}


// One thread per cluster. All threads walk the lights in the same order, so the light addresses are uniform across the wave.
// Each light is read in two halves: the sphere test needs only the first, and the rest is loaded and decoded only when the
// sphere touches a cluster of the wave. The branch is uniform, so the loads stay scalar.
FE_NUM_THREADS(kLightClusterThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const LocalLightingBinding lighting = GConstants.m_lighting;
    const LightClusterData data = lighting.m_clusters.Load();
    const uint clusterIndex = thread.x;
    if (clusterIndex >= data.m_tiles.x * data.m_tiles.y * kLightClusterSlices)
        return;

    const uint tilesPerSlice = data.m_tiles.x * data.m_tiles.y;
    const uint3 cluster =
        uint3(clusterIndex % data.m_tiles.x, (clusterIndex % tilesPerSlice) / data.m_tiles.x, clusterIndex / tilesPerSlice);
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
    const uint base = clusterIndex * data.m_wordCount;
    for (uint word = 0; word < data.m_wordCount; ++word)
    {
        uint mask = 0;
        const uint bitCount = min(32u, data.m_lightCount - word * 32);
        for (uint bit = 0; bit < bitCount; ++bit)
        {
            const BufferPointer address = GetLocalLightAddress(lighting.m_lights, word * 32 + bit);
            const float4 positionRange = LoadLocalLightPositionRange(address);
            const bool touchesSphere = IntersectsSphere(positionRange, lower, upper);
            if (!WaveActiveAnyTrue(touchesSphere))
                continue;

            const DecodedLocalLight light = DecodeLocalLight(LoadLocalLightRemainder(address, positionRange));
            if (touchesSphere && IntersectsCone(light, center, radius))
                mask |= 1u << bit;
        }

        GConstants.m_masks.Store(base + word, mask);
    }
}
