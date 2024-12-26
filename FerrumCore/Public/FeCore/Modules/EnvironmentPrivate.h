#pragma once
#include <FeCore/Console/ConsolePrivate.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/Environment.h>

namespace FE::DI
{
    struct ServiceRegistry;
}


namespace FE::Env::Internal
{
    DI::ServiceRegistry* CreateServiceRegistry();
    DI::ServiceRegistry* GetRootServiceRegistry();


    // Shared private environment data, such as thread data pool or console state.
    struct SharedState final
    {
        Memory::PoolAllocator m_threadDataAllocator;
        Console::ConsoleState m_consoleState{};

        SharedState();

        static SharedState& Get();
    };
} // namespace FE::Env::Internal
