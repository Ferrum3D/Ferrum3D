#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Base/PlatformInclude.h>

FE_PUSH_MSVC_WARNING(4239)
#include <vulkan/vulkan.hpp>
FE_POP_MSVC_WARNING

#include <FeCore/Console/FeLog.h>
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

#define FE_VK_ASSERT(stmt)                                                                                                       \
    do                                                                                                                           \
    {                                                                                                                            \
        vk::Result result = stmt;                                                                                                \
        FE_ASSERT_MSG(result == vk::Result::eSuccess, "Vulkan result was {}", vk::to_string(result));                            \
    }                                                                                                                            \
    while (0)

namespace FE::Osmium
{
    constexpr auto RequiredInstanceLayers = std::array{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation" };

    constexpr auto RequiredInstanceExtensions =
        std::array{ VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, FE_VK_SURFACE_EXT };
} // namespace FE::Osmium
