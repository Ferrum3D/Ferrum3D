#include "Common.hlsli"

struct Constants
{
    float4x4 m_matrix;
    RWStructuredBufferDescriptor<float4x4> m_instanceData;
    uint32_t m_offset;
    float2 m_padding;
};

[[vk::push_constant]] Constants GConstants;


FE_NUM_THREADS(1, 1, 1)
void main()
{
    GConstants.m_instanceData.Store(GConstants.m_offset, GConstants.m_matrix);
}
