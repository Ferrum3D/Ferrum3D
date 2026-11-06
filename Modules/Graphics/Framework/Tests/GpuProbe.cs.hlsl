#include <Shaders/Lighting/LocalLightTable.hlsli>
#include <Shaders/Tests/GpuProbe.h>

[[vk::push_constant]] Constants GConstants;

struct MaterialParameters
{
    float4 Tint;
};

struct InstanceParameters
{
    float2 UvScale;
};

FE_NUM_THREADS(1, 1, 1)
void main(uint3 group : SV_GroupID)
{
    if (GConstants.m_writeArguments == 1)
    {
        GConstants.m_words.Store(0, 3);
        GConstants.m_words.Store(1, 1);
        GConstants.m_words.Store(2, 1);
    }
    else if (GConstants.m_writeArguments == 2)
    {
        const MaterialParameters material = GConstants.m_material.Read<MaterialParameters>(0);
        const InstanceParameters instance = GConstants.m_instance.Read<InstanceParameters>(0);
        GConstants.m_words.Store(group.x, asuint(material.Tint.x + instance.UvScale.x));
    }
    else if (GConstants.m_writeArguments == 3)
    {
        LocalLightingBinding binding = (LocalLightingBinding)0;
        binding.m_lights.m_pageTableAddress = GConstants.m_material;
        const DecodedLocalLight light = LoadLocalLight(binding, group.x);
        [unroll] for (uint channel = 0; channel < 4; ++channel)
        {
            GConstants.m_words.Store(group.x * 16 + channel, asuint(light.m_positionRange[channel]));
            GConstants.m_words.Store(group.x * 16 + 4 + channel, asuint(light.m_colorCandela[channel]));
            GConstants.m_words.Store(group.x * 16 + 8 + channel, asuint(light.m_directionOuterCos[channel]));
            GConstants.m_words.Store(group.x * 16 + 12 + channel, asuint(light.m_cone[channel]));
        }
    }
    else
    {
        GConstants.m_words.Store(group.x, 100 + group.x);
    }
}
