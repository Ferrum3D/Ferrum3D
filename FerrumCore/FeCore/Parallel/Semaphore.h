#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    class Semaphore
    {
        void* m_NativeSemaphore;

    public:
        explicit Semaphore(UInt32 initialValue = 0);
        ~Semaphore();

        void Acquire();
        void Release(UInt32 count = 1);
    };
}
