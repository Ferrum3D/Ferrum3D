#include <FeCore/Containers/ByteBuffer.h>
#include <FeCore/Memory/Memory.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void ByteBuffer_Construct(USize size, ByteBuffer* result)
        {
            *result = ByteBuffer(size);
        }

        FE_DLL_EXPORT void ByteBuffer_Destruct(ByteBuffer* self)
        {
            self->~ByteBuffer();
        }

        FE_DLL_EXPORT void ByteBuffer_CopyTo(ByteBuffer* self, ByteBuffer* dest)
        {
            auto length = std::max(self->Size(), dest->Size());
            memcpy(dest->Data(), self->Data(), length);
        }
    }
} // namespace FE
