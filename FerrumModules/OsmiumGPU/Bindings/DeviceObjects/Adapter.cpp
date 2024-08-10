#include <OsGPU/Adapter/IAdapter.h>
#include <OsGPU/Device/IDevice.h>
#include <objbase.h>
#include <strsafe.h>

namespace FE::Osmium
{
    extern "C"
    {
        struct AdapterDescBinding
        {
            const char* Name;
            int32_t Type;
        };

        FE_DLL_EXPORT IDevice* IAdapter_CreateDevice(IAdapter* self)
        {
            return self->CreateDevice().Detach();
        }

        FE_DLL_EXPORT void IAdapter_GetDesc(IAdapter* self, AdapterDescBinding* desc)
        {
            auto d = self->GetDesc();

            // TODO: Get rid of CoTaskMemAlloc() and copy the string only on C# side, since it is constant
            StringSlice name = d.Name;
            auto result = static_cast<LPSTR>(CoTaskMemAlloc(name.Size() + 1));
            StringCchCopyA(result, name.Size() + 1, name.Data());

            desc->Name = result;
            desc->Type = static_cast<int32_t>(d.Type);
        }

        FE_DLL_EXPORT void IAdapter_Destruct(IAdapter* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
