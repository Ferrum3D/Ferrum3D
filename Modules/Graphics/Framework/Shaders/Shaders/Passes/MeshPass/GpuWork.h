#pragma once
#include <Shaders/Scene/ViewData.h>
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

    // Scene tables are shared by all mesh views and passes.
    struct MeshSceneData final
    {
        MeshInstanceTable::Instance m_instances;
        MeshGroupTable::Instance m_groups;
        MeshLodInfoTable::Instance m_lods;
        MaterialInstanceTable::Instance m_materials;
        MeshBatchTable::Instance m_batches;
        MeshMemberTable::Instance m_members;
        uint32_t m_visibilityType;
    };


    struct MeshViewData final
    {
        ViewData m_view;
        StructuredBufferDescriptor<MeshSceneData> m_scene;
        StructuredBufferDescriptor<VisibleInstance> m_visibleInstances;
        StructuredBufferDescriptor<MeshletWorkChunk> m_workChunks;
        StructuredBufferDescriptor<MeshCommandRange> m_commands;
    };


#if !FE_HOST
    MeshSceneData LoadMeshScene(StructuredBufferDescriptor<MeshSceneData> descriptor)
    {
        MeshSceneData scene = descriptor.Load(0);
        scene.m_instances.m_pageTableAddress.m_deviceAddress =
            WaveReadLaneFirst(scene.m_instances.m_pageTableAddress.m_deviceAddress);
        scene.m_groups.m_pageTableAddress.m_deviceAddress = WaveReadLaneFirst(scene.m_groups.m_pageTableAddress.m_deviceAddress);
        scene.m_lods.m_pageTableAddress.m_deviceAddress = WaveReadLaneFirst(scene.m_lods.m_pageTableAddress.m_deviceAddress);
        scene.m_materials.m_pageTableAddress.m_deviceAddress =
            WaveReadLaneFirst(scene.m_materials.m_pageTableAddress.m_deviceAddress);
        scene.m_batches.m_pageTableAddress.m_deviceAddress =
            WaveReadLaneFirst(scene.m_batches.m_pageTableAddress.m_deviceAddress);
        scene.m_members.m_pageTableAddress.m_deviceAddress =
            WaveReadLaneFirst(scene.m_members.m_pageTableAddress.m_deviceAddress);
        scene.m_visibilityType = WaveReadLaneFirst(scene.m_visibilityType);
        return scene;
    }


    // Keep matrix layout identical to the structured-buffer reads used by rasterization.
    MeshViewData LoadMeshView(ByteAddressBufferDescriptor objectData)
    {
        StructuredBufferDescriptor<MeshViewData> view;
        view.m_index = objectData.m_index;
        MeshViewData data = view.Load(0);
        data.m_view = WaveUniformView(data.m_view);
        data.m_scene.m_index = WaveReadLaneFirst(data.m_scene.m_index);
        return data;
    }
#endif
FE_HOST_END_NAMESPACE
