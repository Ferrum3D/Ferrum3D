#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowAllocateConstants GConstants;
groupshared uint2 Prefix[kSunShadowLevelPageCount];

uint GetBlockPriority(const uint block)
{
    if (block < GConstants.m_levelCount)
        return block;

    const uint local = block - GConstants.m_levelCount;
    return GConstants.m_levelCount + (local % kLocalShadowMipCount) * kLocalShadowMaxMapCount + local / kLocalShadowMipCount;
}


// Deterministic ranks reserve coarse coverage first, then fine sun levels and local mips from near to far.
void RankRequests(const uint lane, const uint block)
{
    const uint first = block * kSunShadowLevelPageCount;
    for (uint item = lane; item < kSunShadowLevelPageCount; item += kSunShadowThreadCount)
    {
        const uint entry = GConstants.m_pageTable.Load(first + item);
        const bool isRequested = (entry & kSunShadowPageRequested) != 0;
        const bool needsBacking = GConstants.m_phase == 0 ? isRequested
            : isRequested && (entry & (kSunShadowPageAllocated | kSunShadowPageAdmitted)) == kSunShadowPageAdmitted;
        const bool isCoarse = (entry & kSunShadowPageCoarse) != 0;
        Prefix[item] = uint2(needsBacking && isCoarse ? 1 : 0, needsBacking && !isCoarse ? 1 : 0);
    }

    GroupMemoryBarrierWithGroupSync();
    for (uint offset = 1; offset < kSunShadowLevelPageCount; offset *= 2)
    {
        uint2 values[4];
        for (uint item = 0; item < 4; ++item)
        {
            const uint index = lane + item * kSunShadowThreadCount;
            values[item] = Prefix[index] + (index >= offset ? Prefix[index - offset] : uint2(0, 0));
        }

        GroupMemoryBarrierWithGroupSync();
        for (uint item = 0; item < 4; ++item)
            Prefix[lane + item * kSunShadowThreadCount] = values[item];

        GroupMemoryBarrierWithGroupSync();
    }

    const uint order = GetBlockPriority(block);
    if (lane == 0)
    {
        const uint2 counts = Prefix[kSunShadowLevelPageCount - 1];
        GConstants.m_counters.Store(kSunShadowPendingCounter + order, counts.x);
        GConstants.m_counters.Store(kSunShadowPendingCounter + kSunShadowMaxBlockCount + order, counts.y);
    }

    for (uint item = lane; item < kSunShadowLevelPageCount; item += kSunShadowThreadCount)
    {
        const uint entry = GConstants.m_pageTable.Load(first + item);
        const bool isCoarse = (entry & kSunShadowPageCoarse) != 0;
        const uint slot = order + (isCoarse ? 0 : kSunShadowMaxBlockCount);
        const uint rank = isCoarse ? Prefix[item].x : Prefix[item].y;
        GConstants.m_requestRanks.Store(first + item, (slot << 16) | (rank - 1));
    }
}


uint GetRequestRank(const uint index)
{
    const uint packed = GConstants.m_requestRanks.Load(index);
    uint rank = packed & 0xffffu;
    for (uint slot = 0; slot < (packed >> 16); ++slot)
        rank += GConstants.m_counters.Load(kSunShadowPendingCounter + slot);

    return rank;
}


uint AllocateEntry(RWStructuredBuffer<uint> counters, const uint index, const uint entry)
{
    const uint rank = GetRequestRank(index);
    const uint freeCount = counters[kSunShadowFreeCounter];
    uint physicalPage;
    if (rank < freeCount)
    {
        physicalPage = GConstants.m_freePages.Load(rank);
    }
    else
    {
        const uint evictableIndex = rank - freeCount;
        if (evictableIndex >= counters[kSunShadowEvictableCounter])
        {
            InterlockedAdd(counters[kSunShadowOverflowCounter], 1);
            return entry;
        }

        physicalPage = GConstants.m_evictablePages.Load(evictableIndex);
        GConstants.m_pageTable.Store(GConstants.m_owners.Load(physicalPage), 0);
    }

    const uint allocated = entry | kSunShadowPageAllocated | kSunShadowPageDirty | physicalPage;
    GConstants.m_owners.Store(physicalPage, index);
    GConstants.m_pageTable.Store(index, allocated);

    uint dirtyIndex;
    InterlockedAdd(counters[kSunShadowAllocatedCounter], 1, dirtyIndex);
    GConstants.m_dirtyPages.Store(dirtyIndex, physicalPage);
    return allocated;
}


FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID, uint3 thread : SV_DispatchThreadID)
{
    if (GConstants.m_phase == 0 || GConstants.m_phase == 2)
    {
        RankRequests(lane, group.x);
        return;
    }

    const uint index = thread.x;
    if (index >= GConstants.m_pageCount)
        return;

    RWStructuredBuffer<uint> counters = GConstants.m_counters.Get();
    const uint entry = GConstants.m_pageTable.Load(index);
    const bool isRequested = (entry & kSunShadowPageRequested) != 0;
    if (GConstants.m_phase == 1)
    {
        if (!isRequested)
            return;

        if (GetRequestRank(index) < kSunShadowPhysicalPageCount)
        {
            GConstants.m_pageTable.Store(index, entry | kSunShadowPageAdmitted);
        }
        else
        {
            if ((entry & kSunShadowPageAllocated) != 0)
                GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);

            GConstants.m_pageTable.Store(index, entry & (kSunShadowPageRequested | kSunShadowPageCoarse | kSunShadowPageDynamic));
        }

        return;
    }

    const bool needsBacking = isRequested && (entry & kSunShadowPageAllocated) == 0;
    const bool isAdmitted = (entry & kSunShadowPageAdmitted) != 0;
    const uint result = needsBacking && isAdmitted ? AllocateEntry(counters, index, entry) : entry;
    if (needsBacking && !isAdmitted)
        InterlockedAdd(counters[kSunShadowOverflowCounter], 1);

    const uint block = index / kSunShadowLevelPageCount;
    const bool isDirty = (result & (kSunShadowPageAllocated | kSunShadowPageDirty)) == (kSunShadowPageAllocated | kSunShadowPageDirty);
    const uint dynamicFlags = kSunShadowPageAllocated | kSunShadowPageRequested | kSunShadowPageDynamic;
    const bool isDynamic = (result & dynamicFlags) == dynamicFlags;

    // Requested entries are never evicted by this pass, so only this thread stores them. The views of a skipped block draw
    // again once these counts are read back.
    const SunShadowScroll scroll = GConstants.m_scroll.Load(block);
    const bool isUndrawn = (isDirty && scroll.m_skipsStatic != 0) || (isDynamic && scroll.m_skipsDynamic != 0);
    if (isRequested && isUndrawn)
        GConstants.m_pageTable.Store(index, result | kSunShadowPageUndrawn);

    // Groups cover a quarter of a block, so the entries of a wave belong to one block.
    const uint dynamicCount = WaveActiveCountBits(isDynamic);
    const uint dirtyCount = WaveActiveCountBits(isDirty);
    if (WaveIsFirstLane() && dynamicCount != 0)
        InterlockedAdd(counters[kSunShadowBlockDynamicCounter + block], dynamicCount);
    if (WaveIsFirstLane() && dirtyCount != 0)
        InterlockedAdd(counters[kSunShadowBlockDirtyCounter + block], dirtyCount);
}
