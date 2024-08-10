#include <OsGPU/Adapter/IAdapter.h>
#include <OsGPU/Instance/IInstance.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IInstance_GetAdapters(IInstance* self, IAdapter** adapters, int32_t* size)
        {
            auto& a = self->GetAdapters();
            *size   = static_cast<int32_t>(a.Size());
            if (adapters)
            {
                for (size_t i = 0; i < a.Size(); ++i)
                {
                    adapters[i] = a[i].Detach();
                }
            }
        }

        FE_DLL_EXPORT void IInstance_Destruct(IInstance* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
