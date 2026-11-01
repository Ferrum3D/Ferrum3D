#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

// The depth test must run before the requests, so that surfaces behind opaque geometry request nothing.
[earlydepthstencil]
void main(float4 position : SV_Position, float3 worldPosition : POSITION)
{
    const SunShadowMarkRasterData data = GView.m_passData.Load<SunShadowMarkRasterData>(0);
    SunShadow::RequestPages(data.m_pageTable.Get(), data.m_data.Load(0), worldPosition);
}
