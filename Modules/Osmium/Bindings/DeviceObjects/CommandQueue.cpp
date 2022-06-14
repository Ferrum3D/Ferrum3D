#include <GPU/CommandQueue/ICommandQueue.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void ICommandQueue_Destruct(ICommandQueue* self)
        {
            self->ReleaseStrongRef();
        }
    }
}
