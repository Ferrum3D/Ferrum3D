#include <GPU/Descriptors/IDescriptorHeap.h>
#include <GPU/Descriptors/IDescriptorTable.h>

namespace FE::GPU
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
