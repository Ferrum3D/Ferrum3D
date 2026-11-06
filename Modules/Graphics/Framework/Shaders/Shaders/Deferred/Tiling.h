#pragma once
#include <Shaders/Deferred/Contracts.h>
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    FE_CONSTEXPR uint32_t kSurfaceBlockSize = 256;

    struct ShaderRange final
    {
        uint32_t m_firstWave;
        uint32_t m_waveCount;
    };


    // Resolution-independent routing; tile offsets are computed by the classifier.
    struct SurfaceRoute final
    {
        uint32_t m_firstBinding;
        uint32_t m_bindingIndex;
        uint32_t m_bindingCount;
    };


    //! The compact pixel lists and wave commands of one evaluation or shading shader.
    struct SurfaceWorkBinding final
    {
        StructuredBufferDescriptor<uint32_t> m_pixels;
        StructuredBufferDescriptor<WaveCommand> m_waves;
        StructuredBufferDescriptor<ShaderRange> m_ranges;
        uint2 m_resolution;
        uint32_t m_tilesPerRow;
        uint32_t m_shaderSlot;
        FE_RTTI_Reflect("6B2D8F14-3A97-4C05-B1E8-7D4C92A0F6E3");
    };


    //! How classification maps pixels to the buckets of a surface work.
    struct ClassificationGrid final
    {
        uint2 m_resolution;
        uint32_t m_tilesPerRow;
        uint32_t m_tileCount;
        uint32_t m_groupRowCount;
    };


    struct ClearSurfaceConstants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_counts;
        RWStructuredBufferDescriptor<uint32_t> m_cursors;
        uint32_t m_bucketCount;

        FE_RTTI_Reflect("F3A593E6-580F-4345-A508-523659F30882");
    };


    struct EvaluationCountConstants final
    {
        AnyConstantsPtr m_view;
        Texture2DDescriptor<uint2> m_visibility;
        StructuredBufferDescriptor<SurfaceRoute> m_lookup;
        RWStructuredBufferDescriptor<uint32_t> m_counts;
        ClassificationGrid m_grid;

        FE_RTTI_Reflect("E565E335-5FCC-4D69-866B-676D0DA1ACDA");
    };


    struct EvaluationScatterConstants final
    {
        AnyConstantsPtr m_view;
        Texture2DDescriptor<uint2> m_visibility;
        StructuredBufferDescriptor<SurfaceRoute> m_lookup;
        StructuredBufferDescriptor<uint2> m_offsets;
        RWStructuredBufferDescriptor<uint32_t> m_cursors;
        RWStructuredBufferDescriptor<uint32_t> m_pixels;
        ClassificationGrid m_grid;

        FE_RTTI_Reflect("E4BA6E37-1669-414A-98E7-A2F1FBDE4ADA");
    };


    struct ShadingCountConstants final
    {
        Texture2DDescriptor<uint32_t> m_g0Read;
        StructuredBufferDescriptor<SurfaceRoute> m_lookup;
        RWStructuredBufferDescriptor<uint32_t> m_counts;
        ClassificationGrid m_grid;

        FE_RTTI_Reflect("6CF96F7D-92DD-43F5-8C14-FE570FA2880A");
    };


    struct ShadingScatterConstants final
    {
        Texture2DDescriptor<uint32_t> m_g0Read;
        StructuredBufferDescriptor<SurfaceRoute> m_lookup;
        StructuredBufferDescriptor<uint2> m_offsets;
        RWStructuredBufferDescriptor<uint32_t> m_cursors;
        RWStructuredBufferDescriptor<uint32_t> m_pixels;
        ClassificationGrid m_grid;

        FE_RTTI_Reflect("6E642C0F-1525-4DB2-A021-BA36728F1B91");
    };


    struct SurfaceScanConstants final
    {
        StructuredBufferDescriptor<uint32_t> m_countsRead;
        StructuredBufferDescriptor<uint2> m_scanInput;
        RWStructuredBufferDescriptor<uint2> m_scanOutput;
        RWStructuredBufferDescriptor<uint2> m_scanSums;
        RWStructuredBufferDescriptor<DispatchArguments> m_emissionArguments;
        uint32_t m_scanCount;
        uint32_t m_waveLaneCount;
        uint32_t m_scanBuckets;
        uint32_t m_shaderCount;

        FE_RTTI_Reflect("DD28F073-664F-4175-8B2C-4E588BF3882D");
    };


    struct SurfacePropagateConstants final
    {
        RWStructuredBufferDescriptor<uint2> m_scanOutput;
        StructuredBufferDescriptor<uint2> m_parentOffsets;
        uint32_t m_scanCount;

        FE_RTTI_Reflect("12CA5527-DCD8-448D-9141-19BD100B9D80");
    };


    struct SurfaceCommandsConstants final
    {
        StructuredBufferDescriptor<uint2> m_offsets;
        StructuredBufferDescriptor<uint2> m_totals;
        StructuredBufferDescriptor<uint32_t> m_countsRead;
        StructuredBufferDescriptor<uint32_t> m_bindings;
        StructuredBufferDescriptor<uint2> m_shaderBindings;
        RWStructuredBufferDescriptor<WaveCommand> m_waves;
        RWStructuredBufferDescriptor<ShaderRange> m_ranges;
        RWStructuredBufferDescriptor<DispatchArguments> m_arguments;
        uint32_t m_tileCount;
        uint32_t m_bucketCount;
        uint32_t m_shaderCount;
        uint32_t m_waveLaneCount;

        FE_RTTI_Reflect("339FD18D-EA19-42AA-8F6F-33B9AE92142E");
    };
FE_HOST_END_NAMESPACE
