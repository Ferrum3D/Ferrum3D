#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Base/PlatformInclude.h>

#include <volk.h>

#include <FeCore/Logging/Trace.h>
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
        const VkResult FE_UNIQUE_IDENT(result) = (stmt);                                                                         \
        FE_AssertMsg(FE_UNIQUE_IDENT(result) == VK_SUCCESS, "Vulkan result was {}", VKResultToString(FE_UNIQUE_IDENT(result)));  \
    }                                                                                                                            \
    while (0)

#define FE_ENABLE_IMPL_CAST(objectType)                                                                                          \
    inline static objectType* ImplCast(RHI::objectType* ptr)                                                                     \
    {                                                                                                                            \
        return static_cast<objectType*>(ptr);                                                                                    \
    }                                                                                                                            \
    inline static const objectType* ImplCast(const RHI::objectType* ptr)                                                         \
    {                                                                                                                            \
        return static_cast<const objectType*>(ptr);                                                                              \
    }

#define FE_ENABLE_NATIVE_CAST(objectType)                                                                                        \
    FE_ENABLE_IMPL_CAST(objectType)                                                                                              \
    inline static auto NativeCast(const RHI::objectType* pObject)                                                                \
    {                                                                                                                            \
        return ImplCast(pObject)->GetNative();                                                                                   \
    }


namespace FE::Graphics::Vulkan
{
    inline constexpr auto kRequiredInstanceLayers = std::array{ "VK_LAYER_KHRONOS_validation" };


    inline constexpr auto kRequiredInstanceExtensions = std::array{ VK_KHR_SURFACE_EXTENSION_NAME,
                                                                    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                                                                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                                                    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                                                                    FE_VK_SURFACE_EXT };

    StringSlice VKResultToString(VkResult result);
} // namespace FE::Graphics::Vulkan
