#pragma once
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    //! @brief Creates an OS-specific Vulkan surface for a native window.
    VkResult CreateSurface(VkInstance instance, uint64_t nativeWindow, VkSurfaceKHR* surface);
} // namespace FE::Graphics::Vulkan
