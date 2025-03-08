#pragma once
#include <FeCore/Base/Assert.h>
#include <FeCore/Console/ConsolePrivate.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/Environment.h>
#include <festd/vector.h>

namespace FE::DI
{
    struct ServiceRegistry;
}


namespace FE::Env::Internal
{
    DI::ServiceRegistry* CreateServiceRegistry();
    DI::ServiceRegistry* GetRootServiceRegistry();


    //! @brief Shared private environment data, such as thread data pool or console state.
    struct SharedState final
    {
        Threading::SpinLock m_lock;
        Memory::PoolAllocator m_threadDataAllocator;
        Console::ConsoleState m_consoleState{};
        festd::fixed_vector<Trace::AssertionHandler, Trace::kMaxAssertionHandlers> m_assertionHandlers;

        SharedState();

        static SharedState& Get();
    };
} // namespace FE::Env::Internal
