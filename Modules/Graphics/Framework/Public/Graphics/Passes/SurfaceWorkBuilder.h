#pragma once
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Shaders/Passes/Visibility/Tiling.h>

namespace FE::Graphics::Visibility
{
    struct SurfaceWork final
    {
        Rc<Core::Buffer> m_records[2];
        Rc<Core::Buffer> m_histogram;
        Rc<Core::Buffer> m_offsets;
        Rc<Core::Buffer> m_pixels;
        Rc<Core::Buffer> m_runs;
        Rc<Core::Buffer> m_waves;
        Rc<Core::Buffer> m_ranges;
        Rc<Core::Buffer> m_arguments;
        Rc<Core::Buffer> m_scan;
        Rc<Core::Buffer> m_blockTotals;
        Rc<Core::Buffer> m_blockOffsets;
        uint32_t m_tileCapacity = 0;
        uint32_t m_shaderCount = 0;
        uint64_t m_scratchBytes = 0;
    };


    inline constexpr uint64_t kDefaultSurfaceScratchBudget = 16 * 1024 * 1024;
    // Account for power-of-two allocation rounding of the 24-byte run records, plus sort histograms/scans.
    inline constexpr uint64_t kSurfaceScratchBytesPerTile = 16384 * 72 + 64 * 136;
    [[nodiscard]] uint32_t GetSurfaceTileCapacity(uint32_t shaderCount, uint64_t budget);
    [[nodiscard]] SurfaceWork CreateSurfaceWork(Core::Device* device, uint32_t shaderCount,
                                                uint64_t budget = kDefaultSurfaceScratchBudget);
    void BuildSurfaceWork(Core::FrameGraph& graph, Core::Buffer* view, Core::Texture* visibility, Core::Texture* g0,
                          const SurfaceWork& work, uint32_t firstTile, uint32_t tileCount, uint32_t waveLaneCount);
} // namespace FE::Graphics::Visibility
