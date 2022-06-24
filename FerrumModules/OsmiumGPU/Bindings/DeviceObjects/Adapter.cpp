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
            Int32 Type;
        };

        FE_DLL_EXPORT IDevice* IAdapter_CreateDevice(IAdapter* self)
        {
            return self->CreateDevice().Detach();
        }

        FE_DLL_EXPORT void IAdapter_GetDesc(IAdapter* self, AdapterDescBinding* desc)
        {
            auto d = self->GetDesc();

            auto result = (STRSAFE_LPSTR)CoTaskMemAlloc(d.Name.Size() + 1);
            StringCchCopyA(result, d.Name.Size() + 1, d.Name.Data());

            desc->Name = result;
            desc->Type = static_cast<Int32>(d.Type);
        }

        FE_DLL_EXPORT void IAdapter_Destruct(IAdapter* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
