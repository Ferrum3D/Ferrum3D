#pragma once
#include <FeCore/Math/Rect.h>
#include <Graphics/Core/FrameGraph/Base.h>

namespace FE::Graphics::Tools::Blit
{
    struct Settings
    {
        RectF m_destinationRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    };

    void AddPass(Core::FrameGraph& graph, Core::TextureView src, Core::TextureView dst, Settings settings = {});
} // namespace FE::Graphics::Tools::Blit
