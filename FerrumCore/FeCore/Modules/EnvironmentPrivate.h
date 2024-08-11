#pragma once
#include <FeCore/Modules/Environment.h>

namespace FE::DI
{
    class ServiceRegistry;
}


namespace FE::Env::Internal
{
    DI::ServiceRegistry* CreateServiceRegistry();
    DI::ServiceRegistry* GetRootServiceRegistry();

    Memory::PoolAllocator* GetThreadDataPool();
} // namespace FE::Env::Internal
