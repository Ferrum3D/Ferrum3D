#include <GPU/Device/IDevice.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IDevice_Destruct(IDevice* device)
        {
            device->ReleaseStrongRef();
        }
    }
}
