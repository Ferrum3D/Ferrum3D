﻿#include <FeCore/Threading/Context.h>
#include <fcontext/fcontext.h>

namespace FE::Context
{
    Handle Create(void* pStack, const size_t stackByteSize, const Callback callback)
    {
        const fcontext_t result = make_fcontext(pStack, stackByteSize, reinterpret_cast<pfn_fcontext>(callback));
        return Handle{ reinterpret_cast<uint64_t>(result) };
    }


    TransferParams Switch(const Handle contextHandle, const uintptr_t userData)
    {
        FE_CoreAssertDebug(contextHandle.IsValid());
        FE_CoreAssertDebug(contextHandle.m_value);
        FE_CoreAssertDebug((contextHandle.m_value & ((UINT64_C(1) << 48) - 1)) == contextHandle.m_value);
        const fcontext_t fcontext = reinterpret_cast<const fcontext_t>(contextHandle.m_value);
        const fcontext_transfer_t result = jump_fcontext(fcontext, reinterpret_cast<void*>(userData));
        return festd::bit_cast<TransferParams>(result);
    }
} // namespace FE::Context
