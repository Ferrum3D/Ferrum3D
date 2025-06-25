#pragma once
#include <FeCore/Modules/Environment.h>

namespace FE::Graphics::Core
{
    struct Module final : public Env::Module
    {
        void RegisterServices(const DI::ServiceRegistryBuilder& builder) override;

        FE_DECLARE_MODULE(Module);
    };
} // namespace FE::Graphics::Core
