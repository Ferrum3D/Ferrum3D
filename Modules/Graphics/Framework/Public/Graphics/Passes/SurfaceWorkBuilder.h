#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/RingUploader.h>
#include <Shaders/Deferred/Tiling.h>

namespace FE::Graphics::Visibility
{
    struct SurfaceBinding final
    {
        uint32_t m_group;
        uint32_t m_shaderSlot;
    };


    struct SurfaceScanLevel final
    {
        Rc<Core::Buffer> m_offsets;
        Rc<Core::Buffer> m_sums;
        uint32_t m_count = 0;
    };


    struct SurfaceWork final
    {
        Rc<Core::Buffer> m_lookup;
        Rc<Core::Buffer> m_bindings;
        Rc<Core::Buffer> m_shaderBindings;
        Rc<Core::Buffer> m_counts;
        Rc<Core::Buffer> m_cursors;
        Rc<Core::Buffer> m_pixels;
        Rc<Core::Buffer> m_waves;
        Rc<Core::Buffer> m_ranges;
        Rc<Core::Buffer> m_arguments;
        festd::fixed_vector<SurfaceScanLevel, 4> m_scanLevels;
        festd::vector<Vector2UInt> m_lookupData;
        festd::vector<uint32_t> m_bindingData;
        festd::vector<Vector2UInt> m_shaderBindingData;
        Vector2UInt m_resolution{ 0, 0 };
        uint32_t m_tileCount = 0;
        uint32_t m_bucketCount = 0;
        uint32_t m_waveCapacity = 0;
        uint32_t m_shaderCount = 0;
        uint32_t m_waveLaneCount = 32;
    };


    [[nodiscard]] SurfaceWork CreateSurfaceWork(Core::Device* device, Vector2UInt resolution,
                                                festd::span<const SurfaceBinding> bindings, uint32_t groupRowCount,
                                                uint32_t shaderCount, uint32_t waveLaneCount);
    void BuildSurfaceWork(Core::FrameGraph& graph, Core::RingUploader& uploader, Core::Buffer* view, Core::Texture* visibility,
                          Core::Texture* g0, SurfaceWork& work, bool classifyShading = false,
                          Core::ComputePipeline* countPipeline = nullptr, Core::ComputePipeline* scatterPipeline = nullptr);
} // namespace FE::Graphics::Visibility
