#pragma once
#include <FeCore/Framework/IModule.h>

namespace FE
{
    namespace DI
    {
        class ServiceRegistry;
    }


    class ModuleBase : public IModule
    {
        Rc<DI::ServiceRegistry> m_pRegistry;

    protected:
        void Initialize();

    public:
        FE_RTTI_Class(ModuleBase, "4B13A66B-5E7E-41BE-A0D0-DD51E995764B");

        ModuleBase();
        ~ModuleBase() override;
    };
} // namespace FE
