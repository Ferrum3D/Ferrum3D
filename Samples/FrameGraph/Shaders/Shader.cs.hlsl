struct Constants
{
    float4x4 m_worldMatrix;
    uint m_instanceData;
    float3 m_padding;
};

[[vk::push_constant]] Constants GConstants;


#define NUM_THREADS(x, y, z) [numthreads(x, y, z)]


RWStructuredBuffer<float4x4> GBuffers[] : register(u3);

NUM_THREADS(1, 1, 1)
void main()
{
    GBuffers[GConstants.m_instanceData][0] = GConstants.m_worldMatrix;
}
