#include <OsGPU/Adapter/IAdapter.h>
#include <OsGPU/Instance/IInstance.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IInstance_GetAdapters(IInstance* self, IAdapter** adapters, Int32* size)
        {
            auto& a = self->GetAdapters();
            *size   = static_cast<Int32>(a.size());
            if (adapters)
            {
                for (USize i = 0; i < a.size(); ++i)
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
