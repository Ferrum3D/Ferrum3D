#include "Common.hlsli"

struct Payload
{
    uint m_meshletIndices[kLanesPerWave];
};


// struct VertexInput
// {
//     float3 m_pos;
//     float2 m_uv;
// };

struct VertexInput
{
    float3 m_pos;               // R32G32B32_SFLOAT
    uint m_packedUv;            // R16G16_SFLOAT
    uint m_packedColor;         // R8G8B8A8_UNORM
    uint m_packedNormal;        // A2R10G10B10_UNORM
    uint m_packedTangent;       // A2R10G10B10_UNORM
    uint m_packedBlendWeight;   // A2R10G10B10_UNORM
    uint2 m_packedBlendIndices; // R16G16B16A16_UINT

    float2 UnpackUv() FE_CONST
    {
        return float2(f16tof32(m_packedUv), f16tof32(m_packedUv >> 16));
    }
};


struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
    uint m_index : DEBUG_INDEX;
};


struct MeshletHeader
{
    uint32_t m_vertexCount : 8;
    uint32_t m_vertexOffset : 24;
    uint32_t m_primitiveCount : 8;
    uint32_t m_primitiveOffset : 24;
};


struct MeshLodInfo
{
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    uint32_t m_meshletCount;
    uint32_t m_primitiveCount;
};


struct Constants
{
    ByteAddressBufferDescriptor m_geometry;
    Texture2DDescriptor<float4> m_texture;
    SamplerDescriptor m_sampler;
    StructuredBufferDescriptor<float4x4> m_instanceData;
    MeshLodInfo m_lodInfo;
};

[[vk::push_constant]] Constants GConstants;


PixelAttributes LoadAttributes(const uint32_t vertexIndex)
{
    const VertexInput input = GConstants.m_geometry.Load<VertexInput>(vertexIndex * sizeof(VertexInput));
    const float4x4 worldTransform = GConstants.m_instanceData.Load(0);

    PixelAttributes output;
    output.m_pos = mul(float4(input.m_pos, 1.0f), worldTransform);
    output.m_uv = input.UnpackUv();
    output.m_index = vertexIndex;
    return output;
}


struct PackedTriangle
{
    uint32_t m_index0 : 10;
    uint32_t m_index1 : 10;
    uint32_t m_index2 : 10;
    uint32_t m_padding : 2;
};


uint3 LoadPrimitive(uint32_t primitiveIndex, uint32_t primitivesByteOffset)
{
    const PackedTriangle packedTriangle =
        GConstants.m_geometry.Load<PackedTriangle>(primitiveIndex * sizeof(PackedTriangle) + primitivesByteOffset);
    return uint3(packedTriangle.m_index0, packedTriangle.m_index1, packedTriangle.m_index2);
}


FE_NUM_THREADS(64, 1, 1)
FE_OUTPUT_TOPOLOGY("triangle")
void main(const in uint32_t groupThreadID : SV_GroupThreadID, //
          const in uint32_t groupID : SV_GroupID,             //
          out fe_mesh_vertices PixelAttributes verts[64],     //
          out fe_mesh_indices uint3 tris[64])
{
    // const uint32_t meshletIndex = payload.m_meshletIndices[groupID];
    const uint32_t meshletIndex = groupID;
    if (meshletIndex >= GConstants.m_lodInfo.m_meshletCount)
        return;

    const uint32_t indicesByteOffset = GConstants.m_lodInfo.m_vertexCount * sizeof(VertexInput);
    const uint32_t meshletsByteOffset = indicesByteOffset + GConstants.m_lodInfo.m_indexCount * sizeof(uint32_t);
    const uint32_t primitivesByteOffset = meshletsByteOffset + GConstants.m_lodInfo.m_meshletCount * sizeof(MeshletHeader);

    MeshletHeader meshlet = GConstants.m_geometry.Load<MeshletHeader>(meshletIndex * sizeof(MeshletHeader) + meshletsByteOffset);
    SetMeshOutputCounts(meshlet.m_vertexCount, meshlet.m_primitiveCount);

    if (groupThreadID < meshlet.m_vertexCount)
    {
        const uint32_t index =
            GConstants.m_geometry.Load((meshlet.m_vertexOffset + groupThreadID) * sizeof(uint32_t) + indicesByteOffset);
        PixelAttributes attributes = LoadAttributes(index);
        attributes.m_index = meshletIndex;
        verts[groupThreadID] = attributes;
    }

    if (groupThreadID < meshlet.m_primitiveCount)
    {
        const uint3 primitive = LoadPrimitive(meshlet.m_primitiveOffset + groupThreadID, primitivesByteOffset);
        tris[groupThreadID] = primitive;
    }
}
