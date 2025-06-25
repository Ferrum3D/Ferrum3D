#include <Shaders/Features/Tools/Downsample/Downsample.h>

[[vk::push_constant]] Constants GConstants;

uint32_t Mips()
{
    return GConstants.m_mips;
}

uint32_t NumWorkGroups()
{
    return GConstants.m_numWorkGroups;
}

uint2 WorkGroupOffset()
{
    return GConstants.m_workGroupOffset;
}

float2 InvInputSize()
{
    return GConstants.m_invInputSize;
}

fehalf4 SampleSrcImageH(const float2 uv, const uint32_t slice)
{
    const float2 textureCoord = uv * InvInputSize() + InvInputSize();
    const float4 result = GConstants.m_input.SampleLevel(GConstants.m_linearClamp.Get(), float3(textureCoord, slice), 0);
    return fehalf4(result);
}

fehalf4 LoadSrcImageH(const float2 uv, const uint32_t slice)
{
    return fehalf4(GConstants.m_input.Load(uint4(uv, slice, 0)));
}

void StoreSrcMipH(const fehalf4 value, const int2 uv, const uint32_t slice, const uint32_t mip)
{
    GConstants.m_inputSrcMips[mip - 1].Store(uint3(uv, slice), float4(value));
}

fehalf4 LoadMidMipH(const int2 uv, const uint32_t slice)
{
    return fehalf4(GConstants.m_inputSrcMidMip.Load(uint3(uv, slice)));
}

void StoreMidMipH(fehalf4 value, const int2 uv, const uint32_t slice)
{
    GConstants.m_inputSrcMidMip.Store(uint3(uv, slice), float4(value));
}


float4 SampleSrcImage(const int2 uv, const uint32_t slice)
{
    const float2 textureCoord = float2(uv) * InvInputSize() + InvInputSize();
    const float4 result = GConstants.m_input.SampleLevel(GConstants.m_linearClamp.Get(), float3(textureCoord, slice), 0);
    return result;
}

float4 LoadSrcImage(const int2 uv, const uint32_t slice)
{
    return GConstants.m_input.Load(uint4(uv, slice, 0));
}

void StoreSrcMip(float4 value, const int2 uv, const uint32_t slice, const uint32_t mip)
{
    GConstants.m_inputSrcMips[mip - 1].Store(uint3(uv, slice), value);
}

float4 LoadMidMip(const int2 uv, const uint32_t slice)
{
    return GConstants.m_inputSrcMidMip.Load(uint3(uv, slice));
}

void StoreMidMip(float4 value, const int2 uv, const uint32_t slice)
{
    GConstants.m_inputSrcMidMip.Store(uint3(uv, slice), value);
}


void IncreaseAtomicCounter(const in uint32_t slice, inout uint32_t counter)
{
    InterlockedAdd(GConstants.m_internalGlobalAtomic.Get()[0].m_counter[slice], 1, counter);
}

void ResetAtomicCounter(const in uint32_t slice)
{
    GConstants.m_internalGlobalAtomic.Get()[0].m_counter[slice] = 0;
}


#define FFX_GPU 1
#define FFX_HLSL 1

#if FFX_SPD_OPTION_DOWNSAMPLE_FILTER == 0
#    define FFX_SPD_OPTION_LINEAR_SAMPLE 1
#endif

#ifdef __hlsl_dx_compiler
#    pragma dxc diagnostic push
#    pragma dxc diagnostic ignored "-Wambig-lit-shift"
#endif //__hlsl_dx_compiler
#include "ffx_core.h"
#ifdef __hlsl_dx_compiler
#    pragma dxc diagnostic pop
#endif //__hlsl_dx_compiler

#include "spd/ffx_spd_downsample.h"

FE_NUM_THREADS(256, 1, 1)
void main(const uint localThreadIndex : SV_GroupIndex, const uint3 workGroupIndex : SV_GroupID)
{
    DOWNSAMPLE(localThreadIndex, workGroupIndex);
}
