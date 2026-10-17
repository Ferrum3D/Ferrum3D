#include <Shaders/Deferred/Tiling.h>

[[vk::push_constant]] SurfaceCommandsConstants GConstants;

uint2 Offset(uint bucket)
{
    return bucket == GConstants.m_bucketCount ? GConstants.m_totals.Load(0) : GConstants.m_offsets.Load(bucket);
}


FE_NUM_THREADS(256, 1, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint index = dispatch.x;
    if (index < GConstants.m_shaderCount)
    {
        const uint2 binding = GConstants.m_shaderBindings.Load(index);
        ShaderRange range;
        range.m_firstWave = Offset(binding.x * GConstants.m_tileCount).y;
        range.m_waveCount = Offset((binding.x + binding.y) * GConstants.m_tileCount).y - range.m_firstWave;
        GConstants.m_ranges.Store(index, range);
        DispatchArguments arguments;
        arguments.m_x = range.m_waveCount;
        arguments.m_y = 1;
        arguments.m_z = 1;
        GConstants.m_arguments.Store(index, arguments);
    }

    if (index >= GConstants.m_totals.Load(0).y)
        return;

    uint lo = 0;
    uint hi = GConstants.m_bucketCount;
    while (lo < hi)
    {
        const uint mid = (lo + hi) / 2;
        if (GConstants.m_offsets.Load(mid).y <= index)
            lo = mid + 1;
        else
            hi = mid;
    }

    const uint bucket = lo - 1;
    lo = 0;
    hi = GConstants.m_shaderCount;

    while (lo < hi)
    {
        const uint mid = (lo + hi) / 2;
        const uint2 binding = GConstants.m_shaderBindings.Load(mid);
        if ((binding.x + binding.y) * GConstants.m_tileCount <= bucket)
            lo = mid + 1;
        else
            hi = mid;
    }

    const uint2 binding = GConstants.m_shaderBindings.Load(lo);
    const uint relative = bucket - binding.x * GConstants.m_tileCount;
    const uint2 offset = GConstants.m_offsets.Load(bucket);
    const uint consumed = (index - offset.y) * GConstants.m_waveLaneCount;
    WaveCommand command;
    command.m_tile = relative / binding.y;
    command.m_group = GConstants.m_bindings.Load(binding.x + relative % binding.y);
    command.m_pixelOffset = offset.x + consumed;
    command.m_validCount = min(GConstants.m_waveLaneCount, GConstants.m_countsRead.Load(bucket) - consumed);
    GConstants.m_waves.Store(index, command);
}
