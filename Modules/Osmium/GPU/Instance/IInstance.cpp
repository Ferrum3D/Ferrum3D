#include <GPU/Instance/IInstance.h>
#include <GPU/Instance/VKInstance.h>

namespace FE::GPU
{
    Shared<IInstance> CreateGraphicsAPIInstance(InstanceDesc desc, GraphicsAPI api)
    {
        switch (api)
        {
        case GraphicsAPI::None:
            break;
        case GraphicsAPI::Vulkan:
            return static_pointer_cast<IInstance>(MakeShared<VKInstance>(desc));
        default:
            break;
        }
        return {};
    }
}
