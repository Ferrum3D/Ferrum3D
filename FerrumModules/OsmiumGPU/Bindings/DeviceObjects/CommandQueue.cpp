#include <Bindings/Common.h>
#include <FeCore/Containers/IByteBuffer.h>
#include <GPU/CommandQueue/ICommandQueue.h>
#include <GPU/Fence/IFence.h>

namespace FE::GPU
{
    extern "C"
    {
        FE_DLL_EXPORT void ICommandQueue_SubmitBuffers(
            ICommandQueue* self, IByteBuffer* buffers, IFence* signalFence, SubmitFlags flags)
        {
            List<ICommandBuffer*> c;
            CopyFromByteBuffer(buffers, c);
            self->SubmitBuffers(c, signalFence, flags);
        }

        FE_DLL_EXPORT void ICommandQueue_Destruct(ICommandQueue* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::GPU
