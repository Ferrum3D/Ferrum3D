#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.hpp>
#undef CopyMemory

#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <array>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#    define FE_VK_SURFACE_EXT VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#    define FE_VK_SURFACE_EXT VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#    define FE_VK_SURFACE_EXT VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

namespace FE::GPU
{
    constexpr auto RequiredInstanceLayers = std::array{ "VK_LAYER_KHRONOS_validation" };

    constexpr auto RequiredInstanceExtensions =
        std::array{ VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, FE_VK_SURFACE_EXT };

    constexpr auto RequiredDeviceExtensions =
        std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
                    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME };
} // namespace FE::GPU
