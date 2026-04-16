#pragma once
#include <Shaders/Core/Meshlet.hlsli>
#include <Shaders/Passes/MeshPass/MeshPass.h>
#include <Shaders/Tables/MeshGroupTable.hlsli>
#include <Shaders/Tables/MeshInstanceTable.hlsli>
#include <Shaders/Tables/MeshLodInfoTable.hlsli>

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
    uint m_meshletIndex : DEBUG_INDEX;
};

struct MeshDrawData
{
    BufferPointer m_geometry;
    float4x4 m_worldTransform;
    Core::MeshLodInfo m_lodInfo;
};

[[vk::push_constant]] Constants GConstants;

MeshDrawData LoadMeshDrawData()
{
    MeshInstanceTable instanceTable = MeshInstanceTable::Create({ GConstants.m_meshInstanceTable });
    MeshGroupTable groupTable = MeshGroupTable::Create({ GConstants.m_meshGroupTable });
    MeshLodInfoTable lodTable = MeshLodInfoTable::Create({ GConstants.m_meshLodInfoTable });

    const MeshInstanceTable::Row instance = instanceTable.ReadRow(GConstants.m_instanceIndex);
    const MeshGroupTable::Row group = groupTable.ReadRow(instance.m_meshGroup.Get());
    const DB::Slice<MeshLodInfoTable> lods = group.m_lods.Get();
    const MeshLodInfoTable::Row lod = lodTable.ReadRow(lods.m_rowIndex);

    MeshDrawData result;
    result.m_geometry = group.m_geometry.Get();
    result.m_worldTransform = instance.m_transform.Get();
    result.m_lodInfo = lod.m_info.Get();
    return result;
}

PixelAttributes LoadAttributes(const MeshDrawData drawData, const uint32_t vertexIndex)
{
    const VertexInput input = drawData.m_geometry.Read<VertexInput>(vertexIndex * sizeof(VertexInput));
    const float4 worldPosition = mul(float4(input.m_pos, 1.0f), drawData.m_worldTransform);
    const float3x3 normalMatrix = transpose(inverse((float3x3)drawData.m_worldTransform));

    PixelAttributes output;
    output.m_pos = mul(worldPosition, GConstants.m_viewProjection);
    output.m_worldPos = worldPosition.xyz;
    output.m_normal = mul(input.UnpackNormal(), normalMatrix);
    output.m_meshletIndex = 0;
    return output;
}

uint3 LoadPrimitive(const MeshDrawData drawData, const uint32_t primitiveIndex, const uint32_t primitivesByteOffset)
{
    const Core::PackedTriangle packedTriangle =
        drawData.m_geometry.Read<Core::PackedTriangle>(primitiveIndex * sizeof(Core::PackedTriangle) + primitivesByteOffset);
    return uint3(packedTriangle.m_index0, packedTriangle.m_index1, packedTriangle.m_index2);
}
