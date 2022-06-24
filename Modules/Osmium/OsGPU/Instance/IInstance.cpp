#include <OsGPU/Instance/IInstance.h>
#include <OsGPU/Instance/VKInstance.h>

namespace FE::GPU
{
    extern "C" FE_DLL_EXPORT void AttachEnvironment(Env::Internal::IEnvironment* environment)
    {
        Env::AttachEnvironment(*environment);
    }

    extern "C" FE_DLL_EXPORT IInstance* CreateGraphicsAPIInstance(InstanceDesc desc, GraphicsAPI api)
    {
        switch (api)
        {
        case GraphicsAPI::None:
            break;
        case GraphicsAPI::Vulkan:
            return static_pointer_cast<IInstance>(MakeShared<VKInstance>(desc)).Detach();
        default:
            break;
        }
        return {};
    }
} // namespace FE::GPU
