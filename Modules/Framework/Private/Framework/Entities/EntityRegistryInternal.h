#pragma once
#include <Core/Threading/SpinLock.h>
#include <festd/bit_vector.h>
#include <festd/vector.h>

namespace FE::Framework::Internal
{
    struct RegistryIDPool
    {
        ~RegistryIDPool();
        uint32_t Allocate();
        void Retain(uint32_t id);
        void Release(uint32_t id);

    private:
        Threading::SpinLock m_lock;
        festd::bit_vector m_available;
        // Recorded commands can outlive a registry; retain its ID until their last reference is gone.
        festd::vector<uint32_t> m_references;
    };
} // namespace FE::Framework::Internal
