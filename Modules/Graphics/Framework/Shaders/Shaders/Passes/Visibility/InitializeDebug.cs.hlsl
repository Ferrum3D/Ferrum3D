#include <Shaders/Passes/Visibility/Debug.h>
#include <Shaders/Passes/Visibility/DebugCommon.hlsli>

[[vk::push_constant]] DebugConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (all(pixel.xy < GConstants.m_resolution))
        GConstants.m_output.Store(pixel.xy, kDebugBackgroundColor);
}
