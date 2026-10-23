#pragma once
#include <Shaders/Core/Culling/Culling.hlsli>
#include <Shaders/Passes/MeshPass/Geometry.hlsli>
#include <Shaders/Passes/MeshPass/GpuWork.h>

float4 LoadMeshletWorldSphere(const MeshDrawData draw, const uint meshletIndex)
{
    const MeshletLayout layout = GetMeshletLayout(draw.m_lodInfo);
    const float4 localSphere = draw.m_geometry.Read<float4>(layout.m_sphereOffset + meshletIndex * sizeof(float4));
    return TransformSphere(localSphere, draw.m_worldTransform);
}


uint LoadMeshletTriangleCount(const MeshDrawData draw, const uint meshletIndex)
{
    const MeshletLayout layout = GetMeshletLayout(draw.m_lodInfo);
    const uint headerOffset = layout.m_headerOffset + meshletIndex * sizeof(Core::MeshletHeader);
    return draw.m_geometry.Read<Core::MeshletHeader>(headerOffset).m_primitiveCount;
}


// Meshlet visibility bits of an instance start at its word offset; returns kInvalidIndex without storage.
uint GetMeshletHistoryWordIndex(const MeshDrawData draw, const uint meshletIndex, const uint capacity)
{
    const uint offset = draw.m_instance.m_meshletVisibilityOffset.Get();
    if (offset == kInvalidIndex)
        return kInvalidIndex;

    const uint wordIndex = offset + meshletIndex / 32;
    return wordIndex < capacity ? wordIndex : kInvalidIndex;
}


// Called by every lane of a wave. Returns the candidate index of each appending lane and grows the indirect arguments of the
// occlusion pass to cover all appended candidates. Group counts only increase, so concurrent maxima stay consistent.
uint AppendOcclusionCandidate(RWStructuredBufferDescriptor<OcclusionCullDispatch> dispatch, const bool isAppended)
{
    const uint appendedCount = WaveActiveCountBits(isAppended);
    uint firstIndex = 0;
    if (WaveIsFirstLane() && appendedCount != 0)
    {
        RWStructuredBuffer<OcclusionCullDispatch> arguments = dispatch.Get();
        InterlockedAdd(arguments[0].m_candidateCount, appendedCount, firstIndex);

        const uint groupCount = (firstIndex + appendedCount + kOcclusionCullThreadCount - 1) / kOcclusionCullThreadCount;
        InterlockedMax(arguments[0].m_groupCountX, min(groupCount, kMaxGroupsPerDimension));
        InterlockedMax(arguments[0].m_groupCountY, (groupCount + kMaxGroupsPerDimension - 1) / kMaxGroupsPerDimension);
    }

    return WaveReadLaneFirst(firstIndex) + WavePrefixCountBits(isAppended);
}


uint GetOcclusionCandidateIndex(const uint lane, const uint3 group)
{
    return (group.y * kMaxGroupsPerDimension + group.x) * kOcclusionCullThreadCount + lane;
}


// Called by every lane of a wave. Appends the meshlets of appending lanes to the meshlet range of their buckets, with one
// atomic per distinct bucket in the wave.
void AppendVisibleMeshlet(RWStructuredBufferDescriptor<uint32_t> meshletCounts,
                          RWStructuredBufferDescriptor<VisibleMeshlet> meshlets,
                          StructuredBufferDescriptor<PipelineBucket> buckets, const VisibleInstance instance,
                          const uint meshletIndex, const bool isAppended)
{
    bool isPending = isAppended;
    while (WaveActiveAnyTrue(isPending))
    {
        const uint bucket = WaveActiveMin(isPending ? instance.m_bucket : kInvalidIndex);
        const bool matches = isPending && instance.m_bucket == bucket;
        const uint matchCount = WaveActiveCountBits(matches);
        const uint prefix = WavePrefixCountBits(matches);

        uint firstOutput = 0;
        if (WaveIsFirstLane())
            InterlockedAdd(meshletCounts.Get()[bucket], matchCount, firstOutput);

        firstOutput = WaveReadLaneFirst(firstOutput);
        if (matches)
        {
            // Each bucket reserves one meshlet slot per meshlet of its work chunks.
            VisibleMeshlet meshlet;
            meshlet.m_instanceId = instance.m_instanceId;
            meshlet.m_lodMeshlet = (instance.m_lodIndex << kVisibleMeshletLodShift) | meshletIndex;
            meshlets.Store(buckets.Load(bucket).m_firstWorkChunk * kMeshletsPerWorkChunk + firstOutput + prefix, meshlet);
        }

        isPending = isPending && !matches;
    }
}


bool HasCullFlag(const MeshCullData cull, const uint32_t flag)
{
    return (cull.m_flags & flag) != 0;
}


struct InstanceBounds
{
    float3 m_center;
    float3 m_extent;
};


InstanceBounds LoadInstanceBounds(const MeshGroupTable::Row group)
{
    const Core::MeshBoundsInfo bounds = group.m_bounds.Get();

    InstanceBounds result;
    result.m_center = (bounds.m_min + bounds.m_max) * 0.5f;
    result.m_extent = (bounds.m_max - bounds.m_min) * 0.5f;
    return result;
}


DepthPyramid CreateDepthPyramid(const Texture2DDescriptor<float> texture, const uint2 size, const float2 uvScale,
                                const uint mipCount)
{
    DepthPyramid pyramid;
    pyramid.m_texture = texture;
    pyramid.m_size = size;
    pyramid.m_uvScale = uvScale;
    pyramid.m_mipCount = mipCount;
    return pyramid;
}
