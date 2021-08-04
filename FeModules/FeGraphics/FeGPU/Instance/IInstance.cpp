#include <FeGPU/Instance/IInstance.h>
#include <FeGPU/Instance/VKInstance.h>

namespace FE::GPU
{
    RefCountPtr<IInstance> CreateGraphicsAPIInstance(InstanceDesc desc, GraphicsAPI api)
    {
        switch (api)
        {
        case GraphicsAPI::None:
            break;
        case GraphicsAPI::Vulkan:
            return StaticPtrCast<IInstance>(MakeShared<VKInstance>(desc));
        default:
            break;
        }
        return {};
    }
}
