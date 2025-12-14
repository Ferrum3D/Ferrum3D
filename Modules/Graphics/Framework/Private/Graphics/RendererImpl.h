#pragma once
#include <Graphics/Renderer.h>

namespace FE::Graphics
{
    struct RendererImpl final : public Renderer
    {
        FE_RTTI("CFD1E397-FC2E-4F9B-99F3-4CF67F695B1E");

        RendererImpl();
        ~RendererImpl() override;
    };
} // namespace FE::Graphics
