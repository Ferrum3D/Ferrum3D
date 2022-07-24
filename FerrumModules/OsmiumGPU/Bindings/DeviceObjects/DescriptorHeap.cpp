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

        FE_DLL_EXPORT void IDescriptorHeap_Reset(IDescriptorHeap* self)
        {
            self->Reset();
        }

        FE_DLL_EXPORT IDescriptorTable* IDescriptorHeap_AllocateDescriptorTable(IDescriptorHeap* self, DescriptorDesc* descs,
                                                                                UInt32 count)
        {
            return self->AllocateDescriptorTable(ArraySlice(descs, descs + count)).Detach();
        }
    }
} // namespace FE::Osmium
