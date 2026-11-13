#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shadows/SunShadow.h>

#ifndef SHADOW_TWO_SIDED
#    define SHADOW_TWO_SIDED 0
#endif

struct ShadowVertex
{
    float4 m_position : SV_Position;
    //! The view of the meshlet, which selects the page table block that the pixel shader writes.
    nointerpolation uint m_viewIndex : VIEW;
};


struct ShadowPrimitive
{
    bool m_culled : SV_CullPrimitive;
};

groupshared float4 GClipPositions[64];
groupshared uint GClipMasks[64];


uint GetClipMask(const float4 position)
{
    const bool3 below = position.xyz < float3(-position.w, -position.w, 0.0f);
    const bool3 above = position.xyz > position.www;
    return uint(below.x) | (uint(above.x) << 1) | (uint(below.y) << 2) | (uint(above.y) << 3) | (uint(below.z) << 4)
        | (uint(above.z) << 5);
}


bool IsBackFacing(const float4 p0, const float4 p1, const float4 p2, const bool isMirrored)
{
    // Keep eye-plane crossings for hardware clipping; division by a nonpositive w can reverse their winding.
    if (any(float3(p0.w, p1.w, p2.w) <= 0.0f))
        return false;

    // With the flipped viewport, negative clip-space area matches the pipeline's clockwise front faces. A mirroring
    // instance transform reverses the winding of its triangles.
    const float area = determinant(float3x3(p0.xyw, p1.xyw, p2.xyw));
    return isMirrored ? area <= 0.0f : area >= 0.0f;
}


// A triangle whose bounds round to the same texel along either axis contains no texel centre, so it writes nothing.
bool CoversNoTexel(const float4 p0, const float4 p1, const float4 p2, const float2 viewSize)
{
    if (any(float3(p0.w, p1.w, p2.w) <= 0.0f))
        return false;

    const float2 texel0 = (p0.xy / p0.w * float2(0.5f, -0.5f) + 0.5f) * viewSize;
    const float2 texel1 = (p1.xy / p1.w * float2(0.5f, -0.5f) + 0.5f) * viewSize;
    const float2 texel2 = (p2.xy / p2.w * float2(0.5f, -0.5f) + 0.5f) * viewSize;
    const float2 boundsMin = min(texel0, min(texel1, texel2));
    const float2 boundsMax = max(texel0, max(texel1, texel2));
    return any(round(boundsMin) == round(boundsMax));
}


FE_NUM_THREADS(kMeshShaderThreadCount, 1, 1)
FE_OUTPUT_TOPOLOGY("triangle")
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID, out ms_vertices ShadowVertex verts[64],
          out ms_indices uint3 tris[64], out ms_primitives ShadowPrimitive prims[64])
{
    const VisibleMeshlet visibleMeshlet = LoadVisibleMeshlet(group.x);
    const uint lodIndex = visibleMeshlet.m_lodMeshlet >> kVisibleMeshletLodShift;
    const uint meshletIndex = visibleMeshlet.m_lodMeshlet & ((1u << kVisibleMeshletLodShift) - 1);
    const MeshDrawData draw = LoadMeshDrawData(visibleMeshlet.m_instanceId, lodIndex);
    const MeshletLayout layout = GetMeshletLayout(draw.m_lodInfo);
    const Core::MeshletHeader header =
        draw.m_geometry.Read<Core::MeshletHeader>(layout.m_headerOffset + meshletIndex * sizeof(Core::MeshletHeader));
    SetMeshOutputCounts(header.m_vertexCount, header.m_primitiveCount);

    // The pass draws the meshlets of every clip level and local map mip of its layer. Each view projects with its own camera
    // into the top left corner of the shared viewport, scaled to the size of the view.
    const MeshViewData view = GConstants.m_view.LoadElement(visibleMeshlet.m_viewIndex);
    const float2 viewSize = view.m_view.m_viewport.zw;
    const float viewScale = viewSize.x / kSunShadowVirtualSize;

    if (lane < header.m_vertexCount)
    {
        const uint vertexIndex = draw.m_geometry.Read<uint>(layout.m_remapOffset + (header.m_vertexOffset + lane) * sizeof(uint));
        const float3 position = draw.m_geometry.Read<float3>(vertexIndex * sizeof(VertexInput));
        const float4 worldPosition = mul(float4(position, 1.0f), draw.m_worldTransform);
        const float4 clipPosition = mul(worldPosition, view.m_view.m_viewProjection);
        GClipPositions[lane] = clipPosition;
        GClipMasks[lane] = GetClipMask(clipPosition);

        const float2 cornerPosition = float2((clipPosition.x + clipPosition.w) * viewScale - clipPosition.w,
                                             clipPosition.y * viewScale + clipPosition.w * (1.0f - viewScale));
        verts[lane].m_position = float4(cornerPosition, clipPosition.zw);
        verts[lane].m_viewIndex = visibleMeshlet.m_viewIndex;
    }

    GroupMemoryBarrierWithGroupSync();

    if (lane < header.m_primitiveCount)
    {
        const uint3 indices = LoadPrimitive(draw, header.m_primitiveOffset + lane, layout.m_triangleOffset);
        const float4 p0 = GClipPositions[indices.x];
        const float4 p1 = GClipPositions[indices.y];
        const float4 p2 = GClipPositions[indices.z];
        const bool isOutsideFrustum = (GClipMasks[indices.x] & GClipMasks[indices.y] & GClipMasks[indices.z]) != 0;
        bool isCulled = isOutsideFrustum || CoversNoTexel(p0, p1, p2, viewSize);
#if !SHADOW_TWO_SIDED
        const bool isMirrored = determinant((float3x3)draw.m_worldTransform) < 0.0f;
        isCulled = isCulled || IsBackFacing(p0, p1, p2, isMirrored);
#endif
        tris[lane] = indices;
        prims[lane].m_culled = isCulled;
    }
}
