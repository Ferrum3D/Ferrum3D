#include <OsGPU/Instance/IInstance.h>
#include <OsGPU/Instance/VKInstance.h>

namespace FE::Osmium
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
            return Rc{ Rc<VKInstance>::DefaultNew(desc) }.Detach();
        default:
            break;
        }
        return {};
    }
} // namespace FE::Osmium
