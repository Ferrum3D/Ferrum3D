#pragma once
#include <Shaders/Passes/MeshPass/Geometry.hlsli>
#include <Shaders/Passes/Visibility/Contracts.h>

struct PerspectiveWeights
{
    float3 m_value;
    float3 m_ddx;
    float3 m_ddy;
};


float Cross2(const float2 a, const float2 b)
{
    return a.x * b.y - a.y * b.x;
}


bool ComputePerspectiveWeights(const float4 clip0, const float4 clip1, const float4 clip2, const float2 ndc,
                               const float2 ndcPerPixel, out PerspectiveWeights result)
{
    result = (PerspectiveWeights)0;
    // Solve in homogeneous space so clipped triangles do not require division by a source vertex's W.
    const float2 a = clip0.xy - ndc * clip0.w;
    const float2 b = clip1.xy - ndc * clip1.w;
    const float2 c = clip2.xy - ndc * clip2.w;
    const float3 numerators = float3(Cross2(b, c), Cross2(c, a), Cross2(a, b));
    const float denominator = dot(numerators, 1.0f);
    if (!isfinite(denominator) || abs(denominator) < 1e-20f)
        return false;

    const float2 ax = float2(-clip0.w * ndcPerPixel.x, 0);
    const float2 bx = float2(-clip1.w * ndcPerPixel.x, 0);
    const float2 cx = float2(-clip2.w * ndcPerPixel.x, 0);
    const float2 ay = float2(0, -clip0.w * ndcPerPixel.y);
    const float2 by = float2(0, -clip1.w * ndcPerPixel.y);
    const float2 cy = float2(0, -clip2.w * ndcPerPixel.y);
    const float3 dx = float3(Cross2(bx, c) + Cross2(b, cx), Cross2(cx, a) + Cross2(c, ax), Cross2(ax, b) + Cross2(a, bx));
    const float3 dy = float3(Cross2(by, c) + Cross2(b, cy), Cross2(cy, a) + Cross2(c, ay), Cross2(ay, b) + Cross2(a, by));
    result.m_value = numerators / denominator;
    result.m_ddx = (dx - result.m_value * dot(dx, 1.0f)) / denominator;
    result.m_ddy = (dy - result.m_value * dot(dy, 1.0f)) / denominator;
    return all(isfinite(result.m_value)) && all(isfinite(result.m_ddx)) && all(isfinite(result.m_ddy));
}


struct ReconstructedAttributes
{
    float3 m_worldPosition;
    float3 m_normal;
    float4 m_tangent;
    float2 m_uv;
    float2 m_uvDdx;
    float2 m_uvDdy;
    float m_depth;
    uint m_group;
};


bool ReconstructAttributes(const MeshViewData view, const uint2 visibility, const float2 pixelCenter, const float2 resolution,
                           out ReconstructedAttributes result, const bool needUV = true, const bool needTangent = true)
{
    result = (ReconstructedAttributes)0;
    if (visibility.x == 0 || (visibility.x >> kObjectTypeShift) != view.m_scene.Load(0).m_visibilityType)
        return false;

    const uint instanceIndex = (visibility.x & kObjectIndexMask) - 1;
    const uint meshletIndex = visibility.y >> kPrimitiveBits;
    const uint primitiveIndex = visibility.y & (kMaxPrimitiveCount - 1);
    const MeshDrawData draw = ResolveMeshDrawData(view, instanceIndex);
    if (meshletIndex >= draw.m_lodInfo.m_meshletCount)
        return false;

    const uint remapOffset = draw.m_lodInfo.m_vertexCount * sizeof(VertexInput);
    const uint headerOffset = remapOffset + draw.m_lodInfo.m_indexCount * sizeof(uint);
    const uint triangleOffset = headerOffset + draw.m_lodInfo.m_meshletCount * sizeof(Core::MeshletHeader);
    const Core::MeshletHeader header =
        draw.m_geometry.Read<Core::MeshletHeader>(headerOffset + meshletIndex * sizeof(Core::MeshletHeader));
    if (primitiveIndex >= header.m_primitiveCount)
        return false;

    const uint3 local = LoadPrimitive(draw, header.m_primitiveOffset + primitiveIndex, triangleOffset);
    const uint i0 = draw.m_geometry.Read<uint>(remapOffset + (header.m_vertexOffset + local.x) * sizeof(uint));
    const uint i1 = draw.m_geometry.Read<uint>(remapOffset + (header.m_vertexOffset + local.y) * sizeof(uint));
    const uint i2 = draw.m_geometry.Read<uint>(remapOffset + (header.m_vertexOffset + local.z) * sizeof(uint));
    const PixelAttributes v0 = TransformAttributes(draw, i0, instanceIndex, view.m_view.m_viewProjection, needUV, needTangent);
    const PixelAttributes v1 = TransformAttributes(draw, i1, instanceIndex, view.m_view.m_viewProjection, needUV, needTangent);
    const PixelAttributes v2 = TransformAttributes(draw, i2, instanceIndex, view.m_view.m_viewProjection, needUV, needTangent);
    const float2 ndcPerPixel = float2(2.0f, -2.0f) / resolution;
    const float2 ndc = pixelCenter * ndcPerPixel + float2(-1.0f, 1.0f);

    PerspectiveWeights weights;
    if (!ComputePerspectiveWeights(v0.m_pos, v1.m_pos, v2.m_pos, ndc, ndcPerPixel, weights))
        return false;

    const float3 w = weights.m_value;
    result.m_worldPosition = v0.m_worldPos * w.x + v1.m_worldPos * w.y + v2.m_worldPos * w.z;
    result.m_normal = normalize(v0.m_normal * w.x + v1.m_normal * w.y + v2.m_normal * w.z);
    result.m_tangent = v0.m_tangent * w.x + v1.m_tangent * w.y + v2.m_tangent * w.z;
    result.m_tangent.xyz = normalize(result.m_tangent.xyz);
    result.m_uv = v0.m_uv * w.x + v1.m_uv * w.y + v2.m_uv * w.z;
    result.m_uvDdx = v0.m_uv * weights.m_ddx.x + v1.m_uv * weights.m_ddx.y + v2.m_uv * weights.m_ddx.z;
    result.m_uvDdy = v0.m_uv * weights.m_ddy.x + v1.m_uv * weights.m_ddy.y + v2.m_uv * weights.m_ddy.z;
    const float4 clip = v0.m_pos * w.x + v1.m_pos * w.y + v2.m_pos * w.z;
    result.m_depth = clip.z / clip.w;
    const MeshInstanceTable::Row instance = MeshInstanceTable::Create(view.m_scene.Load(0).m_instances).ReadRow(instanceIndex);
    result.m_group = instance.m_meshGroup.Get().m_rowIndex;
    return all(isfinite(result.m_normal)) && all(isfinite(result.m_uv));
}


float4 SampleReconstructedTexture(Texture2DDescriptor<float4> texture, SamplerState sampler,
                                  const ReconstructedAttributes attributes, const float2 uvScale)
{
    return texture.SampleGrad(sampler, attributes.m_uv * uvScale, attributes.m_uvDdx * uvScale, attributes.m_uvDdy * uvScale);
}
