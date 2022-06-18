#include <FeCore/Memory/Memory.h>
#include <FeCore/Containers/ByteBuffer.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT IByteBuffer* IByteBuffer_Construct(USize size)
        {
            return MakeShared<ByteBuffer>(size).Detach();
        }

        FE_DLL_EXPORT void IByteBuffer_Destruct(IByteBuffer* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT UInt8* IByteBuffer_Data(IByteBuffer* self)
        {
            return self->Data();
        }

        FE_DLL_EXPORT USize IByteBuffer_Size(IByteBuffer* self)
        {
            return self->Size();
        }
    }
}
