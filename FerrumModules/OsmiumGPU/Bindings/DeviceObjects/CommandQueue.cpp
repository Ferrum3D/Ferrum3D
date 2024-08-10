#include <OsGPU/CommandQueue/ICommandQueue.h>
#include <OsGPU/Fence/IFence.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void ICommandQueue_SubmitBuffers(ICommandQueue* self, ICommandBuffer** buffers, uint32_t bufferCount,
                                                       IFence* signalFence, SubmitFlags flags)
        {
            self->SubmitBuffers(ArraySlice(buffers, buffers + bufferCount), signalFence, flags);
        }

        FE_DLL_EXPORT void ICommandQueue_Destruct(ICommandQueue* self)
        {
            self->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
