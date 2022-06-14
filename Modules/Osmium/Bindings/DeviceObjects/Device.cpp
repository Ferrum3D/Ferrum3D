#include <GPU/Device/IDevice.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT ICommandQueue* IDevice_GetCommandQueue(IDevice* device, Int32 cmdQueueClass)
        {
            return device->GetCommandQueue(static_cast<CommandQueueClass>(cmdQueueClass)).Detach();
        }

        FE_DLL_EXPORT void IDevice_Destruct(IDevice* device)
        {
            device->ReleaseStrongRef();
        }
    }
}
