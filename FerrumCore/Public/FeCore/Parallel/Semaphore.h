#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Threading
{
    struct Semaphore
    {
        explicit Semaphore(uint32_t initialValue = 0);
        ~Semaphore();

        void Acquire();
        void Release(uint32_t count = 1);

    private:
        void* m_nativeSemaphore;
    };
} // namespace FE::Threading
