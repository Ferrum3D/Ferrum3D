#pragma once
#include <DebugHeap.h>
#include <FeCore/Base/Assert.h>
#include <FeCore/Console/ConsolePrivate.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Threading/Mutex.h>
#include <festd/vector.h>

namespace FE::DI
{
    struct ServiceRegistry;
}


namespace FE::Env::Internal
{
    struct DebugHeapHolder final
    {
        Threading::Mutex m_lock;
        DebugHeap* m_heap = nullptr;

        DebugHeapHolder();
        ~DebugHeapHolder();
    };


    //! @brief Shared private environment data, such as thread data pool or console state.
    struct SharedState final
    {
        DebugHeapHolder m_debugHeapHolder;

        Threading::SpinLock m_lock;
        uint64_t m_mainThreadId = 0;
        Memory::PoolAllocator m_threadDataAllocator;
        Console::ConsoleState m_consoleState{};
        festd::fixed_vector<Trace::AssertionHandler, Trace::kMaxAssertionHandlers> m_assertionHandlers;
        festd::array<festd::vector<void*>, 12> m_compressorCache;
        festd::array<festd::vector<void*>, 12> m_gDeflateCompressorCache;
        festd::vector<void*> m_decompressorCache;
        festd::vector<void*> m_gDeflateDecompressorCache;

        SharedState();

        static SharedState& Get();
    };
} // namespace FE::Env::Internal
