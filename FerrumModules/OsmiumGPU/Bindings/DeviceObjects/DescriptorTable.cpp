#include <OsGPU/Descriptors/IDescriptorTable.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IDescriptorTable_Destruct(IDescriptorTable* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT void IDescriptorTable_UpdateBuffer(IDescriptorTable* self, DescriptorWriteBuffer* descriptorWriteBuffer)
        {
            self->Update(*descriptorWriteBuffer);
        }

        FE_DLL_EXPORT void IDescriptorTable_UpdateImage(IDescriptorTable* self, DescriptorWriteImage* descriptorWriteBuffer)
        {
            self->Update(*descriptorWriteBuffer);
        }

        FE_DLL_EXPORT void IDescriptorTable_UpdateSampler(IDescriptorTable* self, DescriptorWriteSampler* descriptorWriteBuffer)
        {
            self->Update(*descriptorWriteBuffer);
        }
    }
}
