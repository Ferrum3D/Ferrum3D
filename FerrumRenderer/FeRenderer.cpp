#include "FeRenderer.h"
#include "FeWindow.h"
#include "FeGraphicsDevice.h"

namespace Ferrum
{
    std::shared_ptr<IFeWindow> FeCreateWindow(uint32_t width, uint32_t height) {
        return std::make_shared<FeWindow>(width, height);
    }

    std::shared_ptr<IFeGraphicsDevice> FeCreateGraphicsDevice(const IFeWindow* window, const FeGraphicsDeviceDesc& desc) {
        auto* w = (FeWindow*)window;
        return std::make_shared<FeGraphicsDevice>(w->GetNativeWindow(), desc);
    }
}
