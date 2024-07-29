#include <FeCore/Parallel/Context.h>
#include <fcontext/fcontext.h>

namespace FE::Context
{
    Handle Create(void* pStack, size_t stackByteSize, Callback callback)
    {
        const fcontext_t result = make_fcontext(pStack, stackByteSize, reinterpret_cast<pfn_fcontext>(callback));
        return Handle{ reinterpret_cast<uint64_t>(result) };
    }


    TransferParams Switch(Handle contextHandle, uintptr_t userData)
    {
        FE_CORE_ASSERT(contextHandle, "Invalid handle");
        FE_CORE_ASSERT(contextHandle.Value, "Invalid handle");
        FE_CORE_ASSERT((contextHandle.Value & ((UINT64_C(1) << 48) - 1)) == contextHandle.Value, "Invalid handle");
        const fcontext_t fcontext = reinterpret_cast<const fcontext_t>(contextHandle.Value);
        const fcontext_transfer_t result = jump_fcontext(fcontext, reinterpret_cast<void*>(userData));
        return bit_cast<TransferParams>(result);
    }
} // namespace FE::Context
