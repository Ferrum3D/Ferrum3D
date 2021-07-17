#include "FeRenderer.h"
#include "FeGraphicsDevice.h"
#include "FeVertexLayout.h"
#include "FeWindow.h"

namespace FE
{
    std::shared_ptr<IFeWindow> CreateMainWindow(uint32_t width, uint32_t height)
    {
        return std::make_shared<FeWindow>(width, height);
    }

    std::shared_ptr<IFeGraphicsDevice> CreateGraphicsDevice(const IFeWindow* window, const FeGraphicsDeviceDesc& desc)
    {
        auto* w = (FeWindow*)window;
        return std::make_shared<FeGraphicsDevice>(w->GetNativeWindow(), desc);
    }

    std::shared_ptr<IFeVertexLayout> CreateVertexLayout(size_t size)
    {
        return std::make_shared<FeVertexLayout>(size);
    }
} // namespace FE
