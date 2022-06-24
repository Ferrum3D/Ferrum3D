#include <GPU/Descriptors/IDescriptorTable.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void IDescriptorTable_Destruct(IDescriptorTable* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT void IDescriptorTable_Update(IDescriptorTable* self, DescriptorWriteBuffer* descriptorWriteBuffer)
        {
            self->Update(*descriptorWriteBuffer);
        }
    }
}
