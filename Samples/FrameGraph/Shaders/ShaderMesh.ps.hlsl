#include "Common.hlsli"

struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float3 m_worldPos : POSITION;
    float2 m_uv : TEXCOORD0;
    float3 m_normal : NORMAL;
    uint m_index : DEBUG_INDEX;
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

void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    const float3 texColor = GConstants.m_texture.Sample(GConstants.m_sampler.Get(), input.m_uv).rgb;

    const float3 idxColor = float3(float(input.m_index & 1), float(input.m_index & 3) / 4, float(input.m_index & 7) / 8);
    const float3 objectColor = lerp(texColor, idxColor, 0.2f);

    const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    const float3 lightPos = float3(2.0f, 0.0f, 0.0f);
    const float3 viewPos = float3(0.0f, 2.5f, -2.0f);
    // const float3 lightPos = viewPos;

    // ambient
    float ambientStrength = 0.1;
    float3 ambient = ambientStrength * lightColor;

    // diffuse
    float3 norm = normalize(input.m_normal);
    float3 lightDir = normalize(lightPos - input.m_worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    float3 viewDir = normalize(viewPos - input.m_worldPos);
    float3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    float3 specular = specularStrength * spec * lightColor;

    float3 result = (ambient + diffuse + specular) * objectColor;
    output = float4(result, 1.0f);
}
