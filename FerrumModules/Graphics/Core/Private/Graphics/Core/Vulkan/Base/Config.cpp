#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    festd::string_view VKResultToString(const VkResult result)
    {
        switch (result)
        {
#define MAKE_STRING(val)                                                                                                         \
    case val:                                                                                                                    \
        return #val

            MAKE_STRING(VK_SUCCESS);
            MAKE_STRING(VK_NOT_READY);
            MAKE_STRING(VK_TIMEOUT);
            MAKE_STRING(VK_EVENT_SET);
            MAKE_STRING(VK_EVENT_RESET);
            MAKE_STRING(VK_INCOMPLETE);
            MAKE_STRING(VK_ERROR_OUT_OF_HOST_MEMORY);
            MAKE_STRING(VK_ERROR_OUT_OF_DEVICE_MEMORY);
            MAKE_STRING(VK_ERROR_INITIALIZATION_FAILED);
            MAKE_STRING(VK_ERROR_DEVICE_LOST);
            MAKE_STRING(VK_ERROR_MEMORY_MAP_FAILED);
            MAKE_STRING(VK_ERROR_LAYER_NOT_PRESENT);
            MAKE_STRING(VK_ERROR_EXTENSION_NOT_PRESENT);
            MAKE_STRING(VK_ERROR_FEATURE_NOT_PRESENT);
            MAKE_STRING(VK_ERROR_INCOMPATIBLE_DRIVER);
            MAKE_STRING(VK_ERROR_TOO_MANY_OBJECTS);
            MAKE_STRING(VK_ERROR_FORMAT_NOT_SUPPORTED);
            MAKE_STRING(VK_ERROR_FRAGMENTED_POOL);
            MAKE_STRING(VK_ERROR_UNKNOWN);
            MAKE_STRING(VK_ERROR_OUT_OF_POOL_MEMORY);
            MAKE_STRING(VK_ERROR_INVALID_EXTERNAL_HANDLE);
            MAKE_STRING(VK_ERROR_FRAGMENTATION);
            MAKE_STRING(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
            MAKE_STRING(VK_ERROR_SURFACE_LOST_KHR);
            MAKE_STRING(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
            MAKE_STRING(VK_SUBOPTIMAL_KHR);
            MAKE_STRING(VK_ERROR_OUT_OF_DATE_KHR);
            MAKE_STRING(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
            MAKE_STRING(VK_ERROR_VALIDATION_FAILED_EXT);
            MAKE_STRING(VK_ERROR_INVALID_SHADER_NV);
            MAKE_STRING(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
            MAKE_STRING(VK_ERROR_NOT_PERMITTED_EXT);
            MAKE_STRING(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
            MAKE_STRING(VK_THREAD_IDLE_KHR);
            MAKE_STRING(VK_THREAD_DONE_KHR);
            MAKE_STRING(VK_OPERATION_DEFERRED_KHR);
            MAKE_STRING(VK_OPERATION_NOT_DEFERRED_KHR);
            MAKE_STRING(VK_PIPELINE_COMPILE_REQUIRED_EXT);
            MAKE_STRING(VK_RESULT_MAX_ENUM);
#undef MAKE_STRING
        default:
            return "< UNKNOWN >";
        }
    }
} // namespace FE::Graphics::Vulkan
