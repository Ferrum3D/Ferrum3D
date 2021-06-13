#pragma once
#include <memory>
#include "FeRenderAPI.h"
#include "IFeWindow.h"
#include "IFeGraphicsDevice.h"

namespace Ferrum
{
	FE_RENDER_API std::shared_ptr<IFeWindow> FeCreateWindow(uint32_t width, uint32_t height);
	FE_RENDER_API std::shared_ptr<IFeGraphicsDevice> FeCreateGraphicsDevice(const IFeWindow* window, const FeGraphicsDeviceDesc& desc);
}
