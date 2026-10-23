#pragma once
#include <Shaders/Base/Packing.hlsli>
#include <Shaders/Core/Meshlet.hlsli>
#include <Shaders/Tables/MaterialInstanceTable.hlsli>
#include <Shaders/Tables/MeshGroupTable.hlsli>
#include <Shaders/Tables/MeshInstanceTable.hlsli>
#include <Shaders/Tables/MeshLodInfoTable.hlsli>

#include <Shaders/Passes/MeshPass/GpuWork.h>

struct VertexInput
{
    float3 m_pos;
    uint m_packedUv;
    uint m_packedColor;
    uint m_packedNormal;
    uint m_packedTangent;
    uint m_packedBlendWeight;
    uint2 m_packedBlendIndices;

    float2 UnpackUv() FE_CONST
    {
        return Math::Pack::RG16FloatToRG32Float(m_packedUv);
    }

    float3 UnpackNormal() FE_CONST
    {
        return Math::Pack::A2R10G10B10UnormToRGBA32Float(m_packedNormal).xyz * 2.0f - 1.0f;
    }
};


struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float3 m_worldPos : POSITION;
    float3 m_normal : NORMAL;
    float4 m_tangent : TANGENT;
    float2 m_uv : TEXCOORD0;
    nointerpolation uint m_instanceIndex : TEXCOORD1;
};


struct MeshDrawData
{
    BufferPointer m_geometry;
    float4x4 m_worldTransform;
    Core::MeshLodInfo m_lodInfo;
    MeshInstanceTable::Row m_instance;
    MeshGroupTable::Row m_group;
};


//! Byte offsets of the meshlet data that follows the vertices of one LOD.
struct MeshletLayout
{
    uint32_t m_remapOffset;
    uint32_t m_headerOffset;
    uint32_t m_triangleOffset;
    uint32_t m_sphereOffset;
};


MeshletLayout GetMeshletLayout(const Core::MeshLodInfo lod)
{
    MeshletLayout layout;
    layout.m_remapOffset = lod.m_vertexCount * sizeof(VertexInput);
    layout.m_headerOffset = layout.m_remapOffset + lod.m_indexCount * sizeof(uint);
    layout.m_triangleOffset = layout.m_headerOffset + lod.m_meshletCount * sizeof(Core::MeshletHeader);
    layout.m_sphereOffset = layout.m_triangleOffset + lod.m_primitiveCount * sizeof(Core::PackedTriangle);
    return layout;
}


//! The geometry pointer addresses the selected LOD inside the mesh buffer.
MeshDrawData CreateMeshDrawData(const MeshInstanceTable::Row instance, const MeshGroupTable::Row group,
                                const MeshLodInfoTable::Row lod)
{
    MeshDrawData result;
    result.m_geometry.SetDeviceAddress(group.m_geometry.Get().GetDeviceAddress() + lod.m_geometryOffset.Get());
    result.m_worldTransform = instance.m_transform.Get();
    result.m_lodInfo = lod.m_info.Get();
    result.m_instance = instance;
    result.m_group = group;
    return result;
}


//! LOD indices are relative to the group's LOD slice, which is ordered from the most detailed LOD.
MeshDrawData ResolveMeshDrawData(const MeshViewData view, const uint32_t instanceIndex, const uint32_t lodIndex)
{
    MeshInstanceTable instanceTable = MeshInstanceTable::Create(view.m_scene.Load(0).m_instances);
    MeshGroupTable groupTable = MeshGroupTable::Create(view.m_scene.Load(0).m_groups);
    MeshLodInfoTable lodTable = MeshLodInfoTable::Create(view.m_scene.Load(0).m_lods);

    const MeshInstanceTable::Row instance = instanceTable.ReadRow(instanceIndex);
    const MeshGroupTable::Row group = groupTable.ReadRow(instance.m_meshGroup.Get());
    const MeshLodInfoTable::Row lod = lodTable.ReadRow(group.m_lods.Get().m_rowIndex + lodIndex);

    return CreateMeshDrawData(instance, group, lod);
}


PixelAttributes TransformAttributes(const MeshDrawData drawData, const uint32_t vertexIndex, const uint32_t instanceIndex,
                                    const float4x4 viewProjection, const bool needUV = true, const bool needTangent = true)
{
    const uint offset = vertexIndex * sizeof(VertexInput);
    const float3 position = drawData.m_geometry.Read<float3>(offset);
    const uint packedNormal = drawData.m_geometry.Read<uint>(offset + 20);
    const float3 inputNormal = Math::Pack::A2R10G10B10UnormToRGBA32Float(packedNormal).xyz * 2.0f - 1.0f;
    const float4 worldPosition = mul(float4(position, 1.0f), drawData.m_worldTransform);
    const float3x3 normalMatrix = (float3x3)drawData.m_worldTransform;

    PixelAttributes output;
    output.m_pos = mul(worldPosition, viewProjection);
    output.m_worldPos = worldPosition.xyz;
    const float3x3 cofactorMatrix = float3x3(cross(normalMatrix[1], normalMatrix[2]),
                                             cross(normalMatrix[2], normalMatrix[0]),
                                             cross(normalMatrix[0], normalMatrix[1]));
    output.m_normal = mul(inputNormal, cofactorMatrix) / determinant(normalMatrix);
    const uint packedTangent = needTangent ? drawData.m_geometry.Read<uint>(offset + 24) : 0;
    const float4 tangent = Math::Pack::A2R10G10B10UnormToRGBA32Float(packedTangent);
    const float handedness = determinant(normalMatrix) < 0.0f ? -1.0f : 1.0f;
    output.m_tangent = float4(mul(tangent.xyz * 2.0f - 1.0f, normalMatrix), (tangent.w > 0.5f ? -1.0f : 1.0f) * handedness);
    const uint packedUV = needUV ? drawData.m_geometry.Read<uint>(offset + 12) : 0;
    output.m_uv = needUV ? Math::Pack::RG16FloatToRG32Float(packedUV) : 0;
    output.m_instanceIndex = instanceIndex;
    return output;
}


uint3 LoadPrimitive(const MeshDrawData drawData, const uint32_t primitiveIndex, const uint32_t primitivesByteOffset)
{
    const Core::PackedTriangle packedTriangle =
        drawData.m_geometry.Read<Core::PackedTriangle>(primitiveIndex * sizeof(Core::PackedTriangle) + primitivesByteOffset);
    return uint3(packedTriangle.m_index0, packedTriangle.m_index1, packedTriangle.m_index2);
}


float3 TangentNormalToWorld(const PixelAttributes input, const float3 tangentNormal)
{
    const float3 normal = normalize(input.m_normal);
    const float3 tangent = normalize(input.m_tangent.xyz);
    const float3 bitangent = cross(normal, tangent) * input.m_tangent.w;
    return normalize(tangentNormal.x * tangent + tangentNormal.y * bitangent + tangentNormal.z * normal);
}
