#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

// Called by every lane of a wave; one atomic per wave accumulates the lanes whose condition holds.
void AddCullStat(RWStructuredBufferDescriptor<uint32_t> stats, const uint32_t stat, const bool condition)
{
    const uint32_t count = WaveActiveCountBits(condition);
    if (WaveIsFirstLane() && count != 0)
        InterlockedAdd(stats.Get()[stat], count);
}


void AddCullStatValue(RWStructuredBufferDescriptor<uint32_t> stats, const uint32_t stat, const uint32_t value)
{
    const uint32_t sum = WaveActiveSum(value);
    if (WaveIsFirstLane() && sum != 0)
        InterlockedAdd(stats.Get()[stat], sum);
}
