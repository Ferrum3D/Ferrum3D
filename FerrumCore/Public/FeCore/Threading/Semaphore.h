#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Threading
{
    struct Semaphore final
    {
        explicit Semaphore(uint32_t initialValue = 0);
        ~Semaphore();

        Semaphore(const Semaphore&) = delete;
        Semaphore(Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
        Semaphore& operator=(Semaphore&&) = delete;

        void Acquire();
        void Release();
        void Release(uint32_t count);

    private:
        intptr_t m_semaphore;
    };
} // namespace FE::Threading
