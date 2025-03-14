#include <FeCore/Base/PlatformInclude.h>
#include <Graphics/Core/Vulkan/Platform/VulkanSurface.h>

namespace FE::Graphics
{
    VkResult Vulkan::CreateSurface(const VkInstance instance, const uint64_t nativeWindow, VkSurfaceKHR* surface)
    {
        FE_PROFILER_ZONE();

        VkWin32SurfaceCreateInfoKHR surfaceCI = {};
        surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCI.hwnd = reinterpret_cast<HWND>(nativeWindow);
        surfaceCI.hinstance = GetModuleHandleW(nullptr);
        return vkCreateWin32SurfaceKHR(instance, &surfaceCI, nullptr, surface);
    }
} // namespace FE::Graphics
