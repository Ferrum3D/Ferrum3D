#pragma once
#include <FeCore/Modules/Environment.h>

namespace FE::Graphics::Core
{
    struct Module final : public Env::Module
    {
        FE_DECLARE_MODULE(Module);

        void RegisterServices(const DI::ServiceRegistryBuilder& builder) override;
    };
} // namespace FE::Graphics::Core
