#pragma once
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    //! @brief Creates an OS-specific Vulkan surface for a native window.
    VkResult CreateSurface(VkInstance instance, uint64_t nativeWindow, VkSurfaceKHR* surface);
    //! Query drawable pixel extents using the native window's DPI awareness, independently of the calling worker.
    VkResult QuerySurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint64_t nativeWindow,
                                      VkSurfaceCapabilitiesKHR* capabilities);
} // namespace FE::Graphics::Vulkan
