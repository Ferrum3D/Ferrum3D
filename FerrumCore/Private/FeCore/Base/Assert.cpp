#include <FeCore/Base/Assert.h>
#include <FeCore/Base/AssertPrivate.h>
#include <FeCore/Threading/SpinLock.h>
#include <festd/vector.h>

namespace FE
{
    namespace
    {
        struct AssertionState final
        {
            Threading::SpinLock m_lock;
            festd::fixed_vector<Trace::AssertionHandler, Trace::kMaxAssertionHandlers> m_assertionHandlers;
        };

        AssertionState* GAssertionState;
    } // namespace


    void Trace::Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_Assert(GAssertionState == nullptr, "Assertions already initialized");
        GAssertionState = Memory::New<AssertionState>(allocator);
    }


    void Trace::Internal::Shutdown()
    {
        FE_Assert(GAssertionState != nullptr, "Assertions not initialized");
        GAssertionState->~AssertionState();
        GAssertionState = nullptr;
    }


    Trace::AssertionHandlerToken Trace::RegisterAssertionHandler(const AssertionHandler handler)
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


    void Trace::UnregisterAssertionHandler(const AssertionHandlerToken token)
    {
        const std::lock_guard lock{ GAssertionState->m_lock };
        GAssertionState->m_assertionHandlers[token.m_value] = nullptr;
    }


    void Trace::AssertionReport(const SourceLocation sourceLocation, const char* message, const uint32_t messageSize)
    {
        if (GAssertionState)
        {
            const std::lock_guard lock{ GAssertionState->m_lock };

            for (const AssertionHandler handler : GAssertionState->m_assertionHandlers)
            {
                if (handler)
                    handler(sourceLocation, message, messageSize);
            }
        }

        Platform::AssertionReport(sourceLocation, message, messageSize);
    }
} // namespace FE
