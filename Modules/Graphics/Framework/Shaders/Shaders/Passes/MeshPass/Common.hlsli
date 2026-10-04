#pragma once
#include <Shaders/Core/Meshlet.hlsli>
#include <Shaders/Tables/MaterialInstanceTable.hlsli>
#include <Shaders/Tables/MeshGroupTable.hlsli>
#include <Shaders/Tables/MeshInstanceTable.hlsli>
#include <Shaders/Tables/MeshLodInfoTable.hlsli>

#include <Shaders/Passes/MeshPass/MeshPass.h>

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
};


[[vk::push_constant]] Constants GConstants;
static const ViewData GView = GConstants.m_view.Load(0);


struct MeshPayload
{
    uint m_instanceId;
    uint m_lodId;
    uint m_meshletIds[kMeshletsPerWorkChunk];
};


MeshDrawData LoadMeshDrawData(const uint32_t instanceIndex, const uint32_t lodIndex = kInvalidIndex)
{
    MeshInstanceTable instanceTable = MeshInstanceTable::Create(GView.m_instances);
    MeshGroupTable groupTable = MeshGroupTable::Create(GView.m_groups);
    MeshLodInfoTable lodTable = MeshLodInfoTable::Create(GView.m_lods);

    const MeshInstanceTable::Row instance = instanceTable.ReadRow(instanceIndex);
    const MeshGroupTable::Row group = groupTable.ReadRow(instance.m_meshGroup.Get());
    const DB::Slice<MeshLodInfoTable> lods = group.m_lods.Get();
    const MeshLodInfoTable::Row lod = lodTable.ReadRow(lodIndex == kInvalidIndex ? lods.m_rowIndex : lodIndex);

    MeshDrawData result;
    result.m_geometry = group.m_geometry.Get();
    result.m_worldTransform = instance.m_transform.Get();
    result.m_lodInfo = lod.m_info.Get();
    return result;
}


PixelAttributes LoadAttributes(const MeshDrawData drawData, const uint32_t vertexIndex, const uint32_t instanceIndex)
{
    const VertexInput input = drawData.m_geometry.Read<VertexInput>(vertexIndex * sizeof(VertexInput));
    const float4 worldPosition = mul(float4(input.m_pos, 1.0f), drawData.m_worldTransform);
    const float3x3 normalMatrix = (float3x3)drawData.m_worldTransform;

    PixelAttributes output;
    output.m_pos = mul(worldPosition, GView.m_viewProjection);
    output.m_worldPos = worldPosition.xyz;
    const float3x3 cofactorMatrix = float3x3(cross(normalMatrix[1], normalMatrix[2]),
                                             cross(normalMatrix[2], normalMatrix[0]),
                                             cross(normalMatrix[0], normalMatrix[1]));
    output.m_normal = mul(input.UnpackNormal(), cofactorMatrix) / determinant(normalMatrix);
    const float4 tangent = Math::Pack::A2R10G10B10UnormToRGBA32Float(input.m_packedTangent);
    const float handedness = determinant(normalMatrix) < 0.0f ? -1.0f : 1.0f;
    output.m_tangent = float4(mul(tangent.xyz * 2.0f - 1.0f, normalMatrix), (tangent.w > 0.5f ? -1.0f : 1.0f) * handedness);
    output.m_uv = input.UnpackUv();
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


template<typename TMaterial, typename TInstance>
void LoadMaterialParameters(const uint32_t instanceIndex, out TMaterial material, out TInstance instanceData)
{
    MeshInstanceTable instances = MeshInstanceTable::Create(GView.m_instances);
    MeshGroupTable groups = MeshGroupTable::Create(GView.m_groups);
    MaterialInstanceTable materials = MaterialInstanceTable::Create(GView.m_materials);
    const MeshInstanceTable::Row instance = instances.ReadRow(instanceIndex);
    const MeshGroupTable::Row group = groups.ReadRow(instance.m_meshGroup.Get());
    const MaterialInstanceTable::Row materialInstance = materials.ReadRow(group.m_materialInstance.Get());
    material = materialInstance.m_materialParameters.Get().Read<TMaterial>(0);
    instanceData = instance.m_instanceData.Get().Read<TInstance>(0);
}
