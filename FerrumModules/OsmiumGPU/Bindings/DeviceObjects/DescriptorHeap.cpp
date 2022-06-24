#include <OsGPU/Descriptors/IDescriptorHeap.h>
#include <OsGPU/Descriptors/IDescriptorTable.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IDescriptorHeap_Destruct(IDescriptorHeap* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT IDescriptorTable* IDescriptorHeap_AllocateDescriptorTable(IDescriptorHeap* self, DescriptorDesc* descs, UInt32 count)
        {
            List<DescriptorDesc> d;
            d.Assign(descs, descs + count);
            return self->AllocateDescriptorTable(d).Detach();
        }
    }
}
