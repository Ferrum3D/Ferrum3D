#pragma once
#include <FeCore/Modules/IModule.h>

namespace FE
{
    namespace DI
    {
        struct ServiceRegistry;
    }


    struct ModuleBase : public IModule
    {
        FE_RTTI_Class(ModuleBase, "4B13A66B-5E7E-41BE-A0D0-DD51E995764B");

        ModuleBase();
        ~ModuleBase() override;

    private:
        void Initialize() override;
        Rc<DI::ServiceRegistry> m_pRegistry;

    protected:
        virtual void RegisterServices(DI::ServiceRegistryBuilder builder) = 0;
    };
} // namespace FE
