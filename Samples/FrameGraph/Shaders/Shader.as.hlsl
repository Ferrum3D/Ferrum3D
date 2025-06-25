#include "Common.hlsli"

bool IsVisible()
{
    // Dummy
    return true;
}


struct Payload
{
    uint m_meshletIndices[kLanesPerWave];
};


struct VertexInput
{
    float3 m_pos;
    float2 m_uv;
};


struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
};


struct MeshletHeader
{
    uint m_vertexCountAndOffset;    // ((count - 1) << 24) | offset
    uint m_primitiveCountAndOffset; // ((count - 1) << 24) | offset

    uint GetVertexCount()
    {
        return (m_vertexCountAndOffset >> 24) + 1;
    }

    uint GetPrimitiveCount()
    {
        return (m_primitiveCountAndOffset >> 24) + 1;
    }

    uint GetVertexOffset()
    {
        return m_vertexCountAndOffset & 0x00ffffff;
    }

    uint GetPrimitiveOffset()
    {
        return m_primitiveCountAndOffset & 0x00ffffff;
    }
};

groupshared Payload GPayload;


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

FE_NUM_THREADS(kLanesPerWave, 1, 1)
void main(const uint dispatchThreadID : SV_DispatchThreadID)
{
    bool isVisible = false;

    if (dispatchThreadID < GConstants.m_lodInfo.m_meshletCount)
        isVisible = IsVisible();

    if (isVisible)
    {
        const uint index = WavePrefixCountBits(isVisible);
        GPayload.m_meshletIndices[index] = dispatchThreadID;
    }

    const uint visibleCount = WaveActiveCountBits(isVisible);
    DispatchMesh(visibleCount, 1, 1, GPayload);
}
