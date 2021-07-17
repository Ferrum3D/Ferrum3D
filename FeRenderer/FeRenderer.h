#pragma once
#include "FeRenderAPI.h"
#include "IFeGraphicsDevice.h"
#include "IFeShader.h"
#include "IFeWindow.h"
#include <FeRenderer/IFeVertexLayout.h>
#include <memory>

namespace FE
{
    FE_RENDER_API std::shared_ptr<IFeWindow> CreateMainWindow(uint32_t width, uint32_t height);
    FE_RENDER_API std::shared_ptr<IFeGraphicsDevice> CreateGraphicsDevice(const IFeWindow* window, const FeGraphicsDeviceDesc& desc);
    FE_RENDER_API std::shared_ptr<IFeVertexLayout> CreateVertexLayout(size_t size = 0);
} // namespace FE
