#include <Shaders/Base/Base.h>

struct CoverageInput
{
    [[vk::ext_decorate(5271)]] nointerpolation uint2 m_visibility : TEXCOORD0;
};


// DXC exposes the mesh per-primitive decoration through SPIR-V attributes.
[[vk::ext_extension("SPV_EXT_mesh_shader")]] [[vk::ext_capability(5283)]]
void main(CoverageInput input, out uint2 output : SV_Target0)
{
    output = input.m_visibility;
}
