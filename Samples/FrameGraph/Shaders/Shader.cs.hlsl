#include "Common.hlsli"

struct Constants
{
    float4x4 m_worldMatrix;
    RWStructuredBufferDescriptor<float4x4> m_instanceData;
    float3 m_padding;
};

[[vk::push_constant]] Constants GConstants;


FE_NUM_THREADS(1, 1, 1)
void main()
{
    GConstants.m_instanceData.Store(0, GConstants.m_worldMatrix);
}
