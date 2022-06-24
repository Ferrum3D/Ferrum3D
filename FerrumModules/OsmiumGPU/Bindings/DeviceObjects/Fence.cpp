#include <OsGPU/Fence/IFence.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IFence_SignalOnCPU(IFence* self)
        {
            self->SignalOnCPU();
        }

        FE_DLL_EXPORT void IFence_WaitOnCPU(IFence* self)
        {
            self->WaitOnCPU();
        }

        FE_DLL_EXPORT void IFence_Reset(IFence* self)
        {
            self->Reset();
        }

        FE_DLL_EXPORT FenceState IFence_GetState(IFence* self)
        {
            return self->GetState();
        }

        FE_DLL_EXPORT void IFence_Destruct(IFence* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
