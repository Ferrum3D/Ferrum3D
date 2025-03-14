#include <FeCore/Base/Assert.h>
#include <FeCore/Modules/EnvironmentPrivate.h>

namespace FE::Trace
{
    AssertionHandlerToken RegisterAssertionHandler(const AssertionHandler handler)
    {
        Env::Internal::SharedState& state = Env::Internal::SharedState::Get();
        const std::lock_guard lock{ state.m_lock };

        for (uint32_t handlerIndex = 0; handlerIndex < state.m_assertionHandlers.size(); ++handlerIndex)
        {
            if (state.m_assertionHandlers[handlerIndex] == nullptr)
            {
                state.m_assertionHandlers[handlerIndex] = handler;
                return AssertionHandlerToken{ handlerIndex };
            }
        }

        state.m_assertionHandlers.push_back(handler);
        return AssertionHandlerToken{ state.m_assertionHandlers.size() - 1 };
    }


    void UnregisterAssertionHandler(const AssertionHandlerToken token)
    {
        Env::Internal::SharedState& state = Env::Internal::SharedState::Get();
        const std::lock_guard lock{ state.m_lock };
        state.m_assertionHandlers[token.m_value] = nullptr;
    }


    void AssertionReport(const SourceLocation sourceLocation, const char* message, const uint32_t messageSize, const bool crash)
    {
        Platform::AssertionReport(sourceLocation, message, messageSize, false);

        if (Env::EnvironmentAttached())
        {
            Env::Internal::SharedState& state = Env::Internal::SharedState::Get();
            const std::lock_guard lock{ state.m_lock };

            for (const AssertionHandler handler : state.m_assertionHandlers)
            {
                if (handler)
                    handler(sourceLocation, message, messageSize);
            }
        }

        if (crash)
            FE_DebugBreak();
    }
} // namespace FE::Trace
