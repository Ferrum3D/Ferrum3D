#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shadows/LocalShadow.hlsli>

// The depth test must run before the requests, so that surfaces behind opaque geometry request nothing.
[earlydepthstencil]
void main(float4 position : SV_Position, float3 worldPosition : POSITION)
{
    const SunShadowMarkRasterData data = GView.m_passData.Load<SunShadowMarkRasterData>(0);
    const SunShadowData shadowData = data.m_data.Load(0);
    RWStructuredBuffer<uint> pageTable = data.m_pageTable.Get();
    SunShadow::RequestPages(pageTable, shadowData, worldPosition);
    LocalShadow::RequestPages(
        pageTable, shadowData, data.m_localLighting, data.m_localMaps, data.m_localLightMaps, worldPosition, position.xy);
}
