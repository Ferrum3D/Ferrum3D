#include <OsGPU/Buffer/IBuffer.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void IBuffer_AllocateMemory(IBuffer* self, Int32 memoryType)
        {
            self->AllocateMemory(static_cast<MemoryType>(memoryType));
        }

        FE_DLL_EXPORT void IBuffer_UpdateData(IBuffer* self, void* data, UInt64 offset, UInt64 size)
        {
            self->UpdateData(data, offset, size);
        }

        FE_DLL_EXPORT void IBuffer_Destruct(IBuffer* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
