#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    class Semaphore
    {
        void* m_NativeSemaphore;

    public:
        explicit Semaphore(uint32_t initialValue = 0);
        ~Semaphore();

        void Acquire();
        void Release(uint32_t count = 1);
    };
} // namespace FE
