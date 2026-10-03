#include <Shaders/Passes/MeshPass/Common.hlsli>

struct MaterialParameters
{
    Texture2DDescriptor<float4> Albedo;
    float4 Tint;
};

struct InstanceParameters
{
    float4 UvScale;
};

void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    MeshInstanceTable instanceTable = MeshInstanceTable::Create(GView.m_instances);
    MeshGroupTable groupTable = MeshGroupTable::Create(GView.m_groups);
    MaterialInstanceTable materialTable = MaterialInstanceTable::Create(GView.m_materials);

    const MeshInstanceTable::Row instance = instanceTable.ReadRow(input.m_instanceIndex);
    const MeshGroupTable::Row group = groupTable.ReadRow(instance.m_meshGroup.Get());
    const MaterialInstanceTable::Row materialInstance = materialTable.ReadRow(group.m_materialInstance.Get());
    const BufferPointer parameters = materialInstance.m_materialParameters.Get();
    const BufferPointer instanceData = instance.m_instanceData.Get();

    const MaterialParameters materialParameters = parameters.Read<MaterialParameters>(0);
    const InstanceParameters instanceParameters = instanceData.Read<InstanceParameters>(0);

    const float4 tint = materialParameters.Tint;
    const float2 uvScale = instanceParameters.UvScale.xy;
    const float4 texel = materialParameters.Albedo.Sample(GLinearWrapSampler, input.m_uv * uvScale);

    const float3 normal = normalize(input.m_normal);
    const float3 lightDirection = normalize(float3(0.4f, 0.7f, -0.2f));
    const float lighting = 0.15f + max(dot(normal, lightDirection), 0.0f) * 0.85f;
    output = float4(texel.rgb * tint.rgb * lighting, texel.a * tint.a);
}
