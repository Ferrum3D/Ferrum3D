#pragma once
#include <Graphics/Core/FrameGraph/Base.h>
#include <festd/vector.h>

namespace FE::Graphics::Tools::Downsample
{
    enum class Filter
    {
        kMean,
        kMin,
        kMax,
        kCount,
    };


    struct Settings final
    {
        Filter m_filter = Filter::kMean;
        uint32_t m_mipCount = kInvalidIndex;
        bool m_allowFloat16 = true;
    };


    inline constexpr uint32_t kMaxMipCount = 12;

    festd::fixed_vector<Core::RenderTargetHandle, kMaxMipCount> AddPass(const Core::FrameGraphBuilder& builder,
                                                                        Core::RenderTargetHandle src, Settings settings = {});
} // namespace FE::Graphics::Tools::Downsample
