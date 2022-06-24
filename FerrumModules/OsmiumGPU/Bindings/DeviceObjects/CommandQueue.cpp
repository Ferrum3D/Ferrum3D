#include <Bindings/Common.h>
#include <FeCore/Containers/IByteBuffer.h>
#include <OsGPU/CommandQueue/ICommandQueue.h>
#include <OsGPU/Fence/IFence.h>

namespace FE::Osmium
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
} // namespace FE::Osmium
