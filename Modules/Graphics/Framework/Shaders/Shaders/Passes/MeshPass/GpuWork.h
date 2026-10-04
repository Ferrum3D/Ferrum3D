#pragma once
#include <Shaders/Base/Base.h>
#if FE_HOST
#    include <Graphics/Tables/MaterialInstanceTable.h>
#    include <Graphics/Tables/MeshBatchTable.h>
#    include <Graphics/Tables/MeshGroupTable.h>
#    include <Graphics/Tables/MeshInstanceTable.h>
#    include <Graphics/Tables/MeshLodInfoTable.h>
#    include <Graphics/Tables/MeshMemberTable.h>
#else
#    include <Shaders/Tables/MaterialInstanceTable.hlsli>
#    include <Shaders/Tables/MeshBatchTable.hlsli>
#    include <Shaders/Tables/MeshGroupTable.hlsli>
#    include <Shaders/Tables/MeshInstanceTable.hlsli>
#    include <Shaders/Tables/MeshLodInfoTable.hlsli>
#    include <Shaders/Tables/MeshMemberTable.hlsli>
#endif

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)
    FE_CONSTEXPR uint32_t kInstancesPerCullGroup = 256;
    FE_CONSTEXPR uint32_t kBucketsPerScanGroup = 256;
    FE_CONSTEXPR uint32_t kMeshShaderThreadCount = 64;
    FE_CONSTEXPR uint32_t kMeshletsPerWorkChunk = 32;
    // Vulkan guarantees this many task groups per dispatch dimension.
    FE_CONSTEXPR uint32_t kWorkChunksPerCommand = 65535;

    struct CullDispatch final
    {
        uint32_t m_batchId;
        uint32_t m_firstMember;
    };


    struct VisibleInstance final
    {
        uint32_t m_instanceId;
        uint32_t m_lodId;
    };

    struct MeshletWorkChunk final
    {
        uint32_t m_visibleInstanceIndex;
        uint32_t m_firstMeshlet;
    };

    struct InstanceClassification final
    {
        uint32_t m_instanceId;
        uint32_t m_lodId;
        uint32_t m_groupId;
        uint2 m_drawTagMask;
        uint32_t m_meshletCount;
    };

    struct PipelineRouting final
    {
        uint32_t m_bucket FE_INIT(kInvalidIndex);
    };

    struct BucketLayout final
    {
        uint32_t m_firstCommand;
        uint32_t m_commandCount;
    };

    struct PipelineBucket final
    {
        uint32_t m_firstInstance;
        uint32_t m_instanceCount;
        uint32_t m_firstWorkChunk;
        uint32_t m_workChunkCount;
    };

    struct MeshCommandRange final
    {
        uint32_t m_firstWorkChunk;
        uint32_t m_workChunkCount;
    };

    struct ViewData final
    {
        float4x4 m_viewProjection;
        float4 m_cameraPosition;
        MeshInstanceTable::Instance m_instances;
        MeshGroupTable::Instance m_groups;
        MeshLodInfoTable::Instance m_lods;
        MaterialInstanceTable::Instance m_materials;
        MeshBatchTable::Instance m_batches;
        MeshMemberTable::Instance m_members;
        StructuredBufferDescriptor<VisibleInstance> m_visibleInstances;
        StructuredBufferDescriptor<MeshletWorkChunk> m_workChunks;
        StructuredBufferDescriptor<MeshCommandRange> m_commands;
    };
FE_HOST_END_NAMESPACE
