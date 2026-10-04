#pragma once
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Tools::Tonemap
{
    // Destination must be UNORM in the sRGB nonlinear presentation color space.
    void AddPass(Core::FrameGraph& graph, Core::TextureView src, Core::TextureView dst);
} // namespace FE::Graphics::Tools::Tonemap
