#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Context
{
    struct Handle final : TypedHandle<Handle, uint64_t>
    {
    };


    struct TransferParams final
    {
        Handle m_contextHandle;
        uintptr_t m_userData = 0;
    };

    using Callback = void (*)(TransferParams);


    //! @brief Create a new context.
    //!
    //! @param pStack        A pointer to the context stack
    //! @param stackByteSize Context stack size in bytes.
    Handle Create(void* pStack, size_t stackByteSize, Callback callback);


    //! @brief Switch to a context.
    //!
    //! @param contextHandle A handle to the context to switch to.
    //! @param userData      User provided pointer to pass to the new context.
    TransferParams Switch(Handle contextHandle, uintptr_t userData);
} // namespace FE::Context
