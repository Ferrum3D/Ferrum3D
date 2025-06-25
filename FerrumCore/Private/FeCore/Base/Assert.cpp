#include <FeCore/Base/Assert.h>
#include <FeCore/Base/AssertPrivate.h>
#include <FeCore/Threading/SpinLock.h>
#include <festd/vector.h>

namespace FE::Trace
{
    namespace
    {
        struct AssertionState final
        {
            Threading::SpinLock m_lock;
            festd::fixed_vector<AssertionHandler, kMaxAssertionHandlers> m_assertionHandlers;
        };

        AssertionState* GAssertionState;
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssert(GAssertionState == nullptr, "Assertions already initialized");
        GAssertionState = Memory::New<AssertionState>(allocator);
    }


    void Internal::Shutdown()
    {
        FE_CoreAssert(GAssertionState != nullptr, "Assertions not initialized");
        GAssertionState->~AssertionState();
        GAssertionState = nullptr;
    }


    AssertionHandlerToken RegisterAssertionHandler(const AssertionHandler handler)
    {
        const std::lock_guard lock{ GAssertionState->m_lock };

        for (uint32_t handlerIndex = 0; handlerIndex < GAssertionState->m_assertionHandlers.size(); ++handlerIndex)
        {
            if (GAssertionState->m_assertionHandlers[handlerIndex] == nullptr)
            {
                GAssertionState->m_assertionHandlers[handlerIndex] = handler;
                return AssertionHandlerToken{ handlerIndex };
            }
        }

        GAssertionState->m_assertionHandlers.push_back(handler);
        return AssertionHandlerToken{ GAssertionState->m_assertionHandlers.size() - 1 };
    }


    void UnregisterAssertionHandler(const AssertionHandlerToken token)
    {
        const std::lock_guard lock{ GAssertionState->m_lock };
        GAssertionState->m_assertionHandlers[token.m_value] = nullptr;
    }


    void AssertionReport(const SourceLocation sourceLocation, const char* message, const uint32_t messageSize, const bool crash)
    {
        Platform::AssertionReport(sourceLocation, message, messageSize, false);

        if (GAssertionState)
        {
            const std::lock_guard lock{ GAssertionState->m_lock };

            for (const AssertionHandler handler : GAssertionState->m_assertionHandlers)
            {
                if (handler)
                    handler(sourceLocation, message, messageSize);
            }
        }

        if (crash)
            FE_DebugBreak();
    }
} // namespace FE::Trace
