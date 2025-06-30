#include "Common.hlsli"

struct Constants
{
    float4x4 m_worldMatrix;
    BufferUAVDescriptor m_instanceData;
    float3 m_padding;
};

[[vk::push_constant]] Constants GConstants;


NUM_THREADS(1, 1, 1)
void main()
{
    RWStructuredBuffer<float4x4> instanceData = ResourceDescriptorHeap[GConstants.m_instanceData.m_index];
    instanceData[0] = GConstants.m_worldMatrix;
}
