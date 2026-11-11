#include <Shaders/Passes/MeshPass/CullCommon.hlsli>
#include <Shaders/Shadows/PageRegions.hlsli>

[[vk::push_constant]] SunShadowLodConstants GConstants;

uint CalculateViewMask(const InstanceClassification item)
{
    const MeshViewData view = GConstants.m_view.Load();
    const MeshSceneData scene = view.m_scene.Load();
    MeshInstanceTable instances = scene.m_instances;
    MeshGroupTable groups = scene.m_groups;
    const MeshInstanceTable::Row instance = instances.ReadRow(item.m_instanceId);
    const InstanceBounds bounds = LoadInstanceBounds(groups.ReadRow(item.m_groupId));
    const float4x4 localToClip = mul(Math::AffineToMatrix(instance.m_transform.Get()), view.m_cull.m_viewProjection);
    ScreenBounds sharedBounds;
    if (!ProjectBox(bounds.m_center, bounds.m_extent, localToClip, sharedBounds))
        return (1u << GConstants.m_levelCount) - 1;

    const bool isStatic = any(item.m_drawTagMask & GConstants.m_staticTag);
    uint mask = 0;
    for (uint level = 0; level < GConstants.m_levelCount; ++level)
    {
        const SunShadowCullLevel data = GConstants.m_levels.LoadElement(level);
        ScreenBounds projected = sharedBounds;
        projected.m_uvMin = sharedBounds.m_uvMin * data.m_uvScaleBias.xy + data.m_uvScaleBias.zw;
        projected.m_uvMax = sharedBounds.m_uvMax * data.m_uvScaleBias.xy + data.m_uvScaleBias.zw;
        if (any(projected.m_uvMax < 0.0f) || any(projected.m_uvMin > 1.0f))
            continue;

        DepthPyramidBinding binding = data.m_dynamicPages;
        if (isStatic)
            binding = data.m_staticPages;

        // The layer of this level is not drawn this frame.
        if (binding.m_mipCount == 0)
            continue;

        const DepthPyramid pages = CreateDepthPyramid(binding);
        if (!IsOccluded(pages, projected))
            mask |= 1u << level;
    }

    return mask;
}


FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID, uint3 thread : SV_DispatchThreadID)
{
    const uint index = thread.x;
    if (index >= GConstants.m_instanceCount)
        return;

    InstanceClassification item = GConstants.m_instances.Load(index);
    if (item.m_meshletCount == 0)
        return;

    if (GConstants.m_checksLods == 2)
    {
        const uint mask = CalculateViewMask(item);
        item.m_flags = (item.m_flags & 0xffffu) | (mask << InstanceFlags::kViewMaskShift);
        GConstants.m_instances.Store(index, item);
        return;
    }

    if (!any(item.m_drawTagMask & GConstants.m_staticTag))
        return;

    if (GConstants.m_checksLods == 1)
    {
        uint previous;
        InterlockedExchange(GConstants.m_history.Get()[item.m_instanceId], item.m_lodIndex + 1, previous);
        if (previous != item.m_lodIndex + 1)
            GConstants.m_changes.Store(item.m_instanceId, GConstants.m_frame);

        return;
    }

    if (GConstants.m_changes.Load(item.m_instanceId) != GConstants.m_frame)
        return;

    const SunShadowBlock block = GConstants.m_blocks.Load(group.y);
    if (block.m_pagesPerAxis == 0)
        return;

    const MeshSceneData scene = GConstants.m_view.Load().m_scene.Load();
    MeshInstanceTable instances = scene.m_instances;
    const MeshInstanceTable::Row instance = instances.ReadRow(item.m_instanceId);
    MeshGroupTable groups = scene.m_groups;
    const MeshGroupTable::Row mesh = groups.ReadRow(item.m_groupId);
    const InstanceBounds bounds = LoadInstanceBounds(mesh);
    const float4x4 localToWorld = Math::AffineToMatrix(instance.m_transform.Get());
    float3 regionMin = 1e30f;
    float3 regionMax = -1e30f;
    for (uint corner = 0; corner < 8; ++corner)
    {
        const float3 sign = float3((corner & 1) != 0 ? 1 : -1, (corner & 2) != 0 ? 1 : -1, (corner & 4) != 0 ? 1 : -1);
        const float3 position = mul(float4(bounds.m_center + sign * bounds.m_extent, 1), localToWorld).xyz;
        regionMin = min(regionMin, position);
        regionMax = max(regionMax, position);
    }

    uint2 pageMin;
    uint2 pageCount;
    if (!ProjectPageRegion(block, regionMin, regionMax, pageMin, pageCount))
        return;

    for (uint pageIndex = 0; pageIndex < pageCount.x * pageCount.y; ++pageIndex)
    {
        const uint2 page = pageMin + uint2(pageIndex % pageCount.x, pageIndex / pageCount.x);
        const uint entryIndex = SunShadow::GetEntryIndex(group.y, SunShadow::GetWorldPage(block.m_pageOrigin, page));
        const uint entry = GConstants.m_pageTable.Load(entryIndex);
        if ((entry & kSunShadowPageAllocated) == 0)
            continue;

        GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);
        GConstants.m_pageTable.Store(entryIndex, entry & (kSunShadowPageRequested | kSunShadowPageCoarse));
    }
}
