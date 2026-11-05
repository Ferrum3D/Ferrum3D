#pragma once
#include <Shaders/Passes/Visibility/Contracts.h>
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
    // Vulkan guarantees this many compute and mesh groups per dispatch dimension.
    FE_CONSTEXPR uint32_t kMaxGroupsPerDimension = 65535;
    FE_CONSTEXPR uint32_t kMeshletsPerCommand = kMaxGroupsPerDimension;
    FE_CONSTEXPR uint32_t kVisibleMeshletLodShift = 28;
#if FE_HOST
    using Visibility::kMaxLodCount;
#endif

    namespace CullFlags
    {
        FE_CONSTEXPR uint32_t kInstanceFrustum = 1u << 0;
        FE_CONSTEXPR uint32_t kInstanceOcclusion = 1u << 1;
        FE_CONSTEXPR uint32_t kMeshletFrustum = 1u << 2;
        FE_CONSTEXPR uint32_t kMeshletOcclusion = 1u << 3;
        FE_CONSTEXPR uint32_t kLodSelection = 1u << 4;
        //! The projected LOD error does not depend on the distance to the camera.
        FE_CONSTEXPR uint32_t kOrthographic = 1u << 5;
    } // namespace CullFlags


    //! A view renders once without occlusion culling, or when it tests occlusion against a frozen depth pyramid. Otherwise the
    //! early phase renders the instances and meshlets visible in the previous frame, and the late phase tests all of them
    //! against a depth pyramid built from the early depth and renders the visible ones the early phase skipped.
    namespace CullPhase
    {
        FE_CONSTEXPR uint32_t kSingle = 0;
        FE_CONSTEXPR uint32_t kEarly = 1;
        FE_CONSTEXPR uint32_t kLate = 2;
    } // namespace CullPhase


    //! Per-view visibility word of an instance row: bit zero is set when the instance passed occlusion culling in the previous
    //! late phase; the bits above store one plus the LOD index that its meshlet visibility bits describe.
    namespace InstanceVisibility
    {
        FE_CONSTEXPR uint32_t kVisible = 1u << 0;
        FE_CONSTEXPR uint32_t kLodShift = 1;
    } // namespace InstanceVisibility


    namespace InstanceFlags
    {
        //! The early phase rendered the instance and its meshlets visible in the previous frame.
        FE_CONSTEXPR uint32_t kRenderedEarly = 1u << 0;
        //! The meshlet visibility bits of the instance describe its selected LOD.
        FE_CONSTEXPR uint32_t kMeshletHistory = 1u << 1;
    } // namespace InstanceFlags


    //! Counters accumulated by the culling shaders of one view and read back by the CPU.
    namespace CullStat
    {
        FE_CONSTEXPR uint32_t kInstanceCandidates = 0;
        FE_CONSTEXPR uint32_t kInstanceFrustumCulled = 1;
        FE_CONSTEXPR uint32_t kInstanceOcclusionCulled = 2;
        FE_CONSTEXPR uint32_t kInstanceRendered = 3;
        FE_CONSTEXPR uint32_t kInstanceRenderedLate = 4;
        FE_CONSTEXPR uint32_t kMeshletCandidates = 5;
        FE_CONSTEXPR uint32_t kMeshletFrustumCulled = 6;
        FE_CONSTEXPR uint32_t kMeshletOcclusionCulled = 7;
        FE_CONSTEXPR uint32_t kMeshletRendered = 8;
        FE_CONSTEXPR uint32_t kTriangleRendered = 9;
        FE_CONSTEXPR uint32_t kLodHistogram = 10;
        FE_CONSTEXPR uint32_t kCount = kLodHistogram + kMaxLodCount;
    } // namespace CullStat


    struct CullDispatch final
    {
        uint32_t m_batchId;
        uint32_t m_firstMember;
    };


    struct VisibleInstance final
    {
        uint32_t m_instanceId;
        uint32_t m_lodId;
        uint32_t m_lodIndex;
        uint32_t m_bucket;
        uint32_t m_flags;
    };


    struct MeshletWorkChunk final
    {
        uint32_t m_visibleInstanceIndex;
        uint32_t m_firstMeshlet;
    };


    //! A meshlet that survived culling; m_lodMeshlet stores the LOD index above kVisibleMeshletLodShift.
    struct VisibleMeshlet final
    {
        uint32_t m_instanceId;
        uint32_t m_lodMeshlet;
    };


    struct InstanceClassification final
    {
        uint32_t m_instanceId;
        uint32_t m_lodId;
        uint32_t m_lodIndex;
        uint32_t m_groupId;
        uint2 m_drawTagMask;
        uint32_t m_meshletCount;
        uint32_t m_flags;
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
        uint32_t m_firstMeshlet;
        uint32_t m_meshletCount;
    };


    //! Indirect arguments of the meshlet culling dispatch, followed by the number of work chunks it covers.
    struct MeshletCullDispatch final
    {
        uint32_t m_groupCountX;
        uint32_t m_groupCountY;
        uint32_t m_groupCountZ;
        uint32_t m_workChunkCount;
    };


    //! Occlusion tests run in a separate pass over the compacted survivors of frustum culling, so waves do not idle on lanes
    //! that frustum culling already rejected.
    FE_CONSTEXPR uint32_t kOcclusionCullThreadCount = 64;
    //! Set in MeshletOcclusionCandidate::m_meshlet when the early phase rendered the meshlet.
    FE_CONSTEXPR uint32_t kCandidateRenderedEarlyBit = 1u << 31;


    //! Indirect arguments of an occlusion culling pass, followed by the number of candidates it covers. Frustum culling
    //! grows them while appending candidates; they start as { 0, 1, 1, 0 }.
    struct OcclusionCullDispatch final
    {
        uint32_t m_groupCountX;
        uint32_t m_groupCountY;
        uint32_t m_groupCountZ;
        uint32_t m_candidateCount;
    };


    struct MeshletOcclusionCandidate final
    {
        uint32_t m_visibleInstanceIndex;
        //! The meshlet index, with kCandidateRenderedEarlyBit set when the early phase rendered it.
        uint32_t m_meshlet;
    };


    //! Culling inputs of one view. Frustum culling and LOD selection may use a frozen camera. Occlusion culling uses the
    //! camera that produced the depth pyramid: the rendered camera, or the frozen one together with its frozen pyramid.
    struct MeshCullData final
    {
        float4x4 m_viewProjection;
        float4x4 m_occlusionViewProjection;
        //! xyz: LOD reference position; w: pixels per unit of error at unit distance divided by the error threshold, or
        //! pixels per unit of error divided by the threshold with CullFlags::kOrthographic.
        float4 m_cameraPosition;
        uint32_t m_flags;
        uint32_t m_padding0;
        uint32_t m_padding1;
        uint32_t m_padding2;
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
        FE_RTTI_Reflect("58299D8D-CAA1-4532-9B1A-FDBA525D1C50");
    };


    //! Shared by the culling, rasterization and surface evaluation passes of one view.
    struct MeshViewData final
    {
        ViewData m_view;
        MeshCullData m_cull;
        ConstantsPtr<MeshSceneData> m_scene;
        FE_RTTI_Reflect("62759A6A-7766-4BD2-A99A-962141CD38E6");
    };


    //! Results of culling for one rasterization pass.
    struct MeshPassData final
    {
        StructuredBufferDescriptor<VisibleMeshlet> m_visibleMeshlets;
        StructuredBufferDescriptor<MeshCommandRange> m_commands;
        //! Constants of the technique role rendering this pass, e.g. the shadow atlas of a clip level.
        AnyConstantsPtr m_passData;
        FE_RTTI_Reflect("A78E5A5C-712C-4CA4-A09A-A356BC3DC60B");
    };


#if FE_HOST
    static_assert(sizeof(MeshCullData) == 160);
#else
    MeshSceneData LoadMeshScene(ConstantsPtr<MeshSceneData> constants)
    {
        MeshSceneData scene = constants.Load();
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


    MeshCullData WaveUniformCull(MeshCullData cull)
    {
        [unroll] for (uint row = 0; row < 4; ++row)
        {
            cull.m_viewProjection[row] = WaveReadLaneFirst(cull.m_viewProjection[row]);
            cull.m_occlusionViewProjection[row] = WaveReadLaneFirst(cull.m_occlusionViewProjection[row]);
        }

        cull.m_cameraPosition = WaveReadLaneFirst(cull.m_cameraPosition);
        cull.m_flags = WaveReadLaneFirst(cull.m_flags);
        return cull;
    }


    MeshViewData LoadMeshView(ConstantsPtr<MeshViewData> constants)
    {
        MeshViewData data = constants.Load();
        data.m_view = WaveUniformView(data.m_view);
        data.m_cull = WaveUniformCull(data.m_cull);
        data.m_scene.m_ptr.m_deviceAddress = WaveReadLaneFirst(data.m_scene.m_ptr.m_deviceAddress);
        return data;
    }


    MeshViewData LoadMeshView(AnyConstantsPtr objectData)
    {
        return LoadMeshView(objectData.As<MeshViewData>());
    }
#endif
FE_HOST_END_NAMESPACE
