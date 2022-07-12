#include <OsGPU/Resource/ITransientResourceHeap.h>

namespace FE::Osmium
{
    FE_DLL_EXPORT void ITransientResourceHeap_Destruct(ITransientResourceHeap* self)
    {
        self->ReleaseStrongRef();
    }

    FE_DLL_EXPORT void ITransientResourceHeap_Allocate(ITransientResourceHeap* self)
    {
        self->Allocate();
    }

    FE_DLL_EXPORT IImage* ITransientResourceHeap_CreateImage(ITransientResourceHeap* self, TransientImageDesc* desc,
                                                             TransientResourceAllocationStats* stats)
    {
        return self->CreateImage(*desc, *stats).Detach();
    }

    FE_DLL_EXPORT IBuffer* ITransientResourceHeap_CreateBuffer(ITransientResourceHeap* self, TransientBufferDesc* desc,
                                                               TransientResourceAllocationStats* stats)
    {
        return self->CreateBuffer(*desc, *stats).Detach();
    }

    FE_DLL_EXPORT void ITransientResourceHeap_ReleaseImage(ITransientResourceHeap* self, UInt64 resourceID)
    {
        return self->ReleaseImage(resourceID);
    }

    FE_DLL_EXPORT void ITransientResourceHeap_ReleaseBuffer(ITransientResourceHeap* self, UInt64 resourceID)
    {
        return self->ReleaseBuffer(resourceID);
    }
} // namespace FE::Osmium
