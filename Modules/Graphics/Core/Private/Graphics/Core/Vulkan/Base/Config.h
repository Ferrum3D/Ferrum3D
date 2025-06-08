#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Base/PlatformInclude.h>

#include <volk.h>

// Must be included after volk.h
#include <vk_mem_alloc.h>

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

#define FE_ENABLE_IMPL_CAST(objectType)                                                                                          \
    inline static objectType* ImplCast(Core::objectType* ptr)                                                                    \
    {                                                                                                                            \
        return static_cast<objectType*>(ptr);                                                                                    \
    }                                                                                                                            \
    inline static const objectType* ImplCast(const Core::objectType* ptr)                                                        \
    {                                                                                                                            \
        return static_cast<const objectType*>(ptr);                                                                              \
    }

#define FE_ENABLE_NATIVE_CAST(objectType)                                                                                        \
    FE_ENABLE_IMPL_CAST(objectType)                                                                                              \
    inline static auto NativeCast(const Core::objectType* pObject)                                                               \
    {                                                                                                                            \
        return ImplCast(pObject)->GetNative();                                                                                   \
    }


namespace FE::Graphics::Vulkan
{
#if !FE_SHIPPING
    inline constexpr auto kRequiredInstanceLayers = std::array{
        "VK_LAYER_KHRONOS_validation",
    };
#else
    inline constexpr auto kRequiredInstanceLayers = std::array<const char*, 0>{};
#endif


    inline constexpr auto kRequiredInstanceExtensions = std::array{
#if !FE_SHIPPING
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        FE_VK_SURFACE_EXT,
    };

    festd::string_view VKResultToString(VkResult result);


    FE_FORCE_INLINE void VerifyVulkan(const VkResult result)
    {
        FE_AssertMsg(result == VK_SUCCESS, "Vulkan result was {}", VKResultToString(result));
    }
} // namespace FE::Graphics::Vulkan
