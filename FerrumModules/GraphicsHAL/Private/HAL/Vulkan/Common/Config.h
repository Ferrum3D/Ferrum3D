#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Base/PlatformInclude.h>

#include <volk.h>

#include <FeCore/Console/FeLog.h>
#include <FeCore/Memory/Memory.h>
#include <array>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#    define FE_VK_SURFACE_EXT VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#    define FE_VK_SURFACE_EXT VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#    define FE_VK_SURFACE_EXT VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

#define VK_FLAGS_NONE 0
#define FE_VK_ASSERT(stmt)                                                                                                       \
    do                                                                                                                           \
    {                                                                                                                            \
        VkResult result = (stmt);                                                                                                \
        FE_ASSERT_MSG(result == VK_SUCCESS, "Vulkan result was {}", VKResultToString(result));                                   \
    }                                                                                                                            \
    while (0)

#define FE_ENABLE_IMPL_CAST(objectType)                                                                                          \
    inline static objectType* ImplCast(HAL::objectType* ptr)                                                                     \
    {                                                                                                                            \
        return static_cast<objectType*>(ptr);                                                                                    \
    }                                                                                                                            \
    inline static const objectType* ImplCast(const HAL::objectType* ptr)                                                         \
    {                                                                                                                            \
        return static_cast<const objectType*>(ptr);                                                                              \
    }

namespace FE::Graphics::Vulkan
{
    constexpr auto RequiredInstanceLayers = std::array{ "VK_LAYER_KHRONOS_validation" };

    constexpr auto RequiredInstanceExtensions = std::array{ VK_KHR_SURFACE_EXTENSION_NAME,
                                                            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                                                            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                                            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                                                            FE_VK_SURFACE_EXT };

    StringSlice VKResultToString(VkResult result);
} // namespace FE::Graphics::Vulkan
