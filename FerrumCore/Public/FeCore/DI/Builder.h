#pragma once
#include <FeCore/DI/Activator.h>
#include <FeCore/DI/Registration.h>
#include <FeCore/DI/Registry.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE::DI
{
    class ServiceRegistryBuilder;


    namespace Internal
    {
        struct ServiceRegistrationSpec final
        {
            ServiceRegistration* pRegistration = nullptr;
            ServiceActivator* pActivator = nullptr;
        };


        class [[nodiscard]] RegistryToBuilder final
        {
            template<class TInterface>
            friend class RegistryBindBuilder;

            ServiceRegistrationSpec m_Target;

            inline explicit RegistryToBuilder(ServiceRegistrationSpec registrationSpec)
                : m_Target(registrationSpec)
            {
            }

        public:
            inline void InScope(Lifetime lifetime)
            {
                m_Target.pRegistration->SetLifetime(lifetime);
            }

            inline void InSingletonScope()
            {
                InScope(Lifetime::Singleton);
            }

            inline void InThreadScope()
            {
                InScope(Lifetime::Thread);
            }

            inline void InTransientScope()
            {
                InScope(Lifetime::Transient);
            }
        };

        template<class TInterface>
        class [[nodiscard]] RegistryBindBuilder final
        {
            static_assert(std::is_base_of_v<Memory::RefCountedObjectBase, TInterface>);

            friend class DI::ServiceRegistryBuilder;
            ServiceRegistrationSpec m_Target;

            inline RegistryBindBuilder(ServiceRegistrationSpec registrationSpec)
                : m_Target(registrationSpec)
            {
            }

        public:
            template<class TImpl>
            inline RegistryToBuilder To()
            {
                if constexpr (!std::is_same_v<TImpl, TInterface>)
                {
                    FE_CORE_ASSERT(fe_typeid<TImpl>() != fe_typeid<TInterface>(), "");
                }

                *m_Target.pActivator = ServiceActivator::CreateForType<TImpl>();
                return RegistryToBuilder(m_Target);
            }

            inline RegistryToBuilder ToFunc(ActivatorFunction&& function)
            {
                *m_Target.pActivator = ServiceActivator::CreateFromFunction(std::forward<ActivatorFunction>(function));
                m_Target.pRegistration->SetFunction(true);
                return RegistryToBuilder(m_Target);
            }

            inline void ToConst(TInterface* pConst)
            {
                auto factory = [pConst](IServiceProvider*, Memory::RefCountedObjectBase** ppResult) {
                    *ppResult = pConst;
                    return ResultCode::Success;
                };

                *m_Target.pActivator = ServiceActivator::CreateFromFunction(factory);
                m_Target.pRegistration->SetLifetime(Lifetime::Singleton);
                m_Target.pRegistration->SetConstant(true);
                pConst->AddRef();
            }

            inline RegistryToBuilder ToSelf()
            {
                *m_Target.pActivator = ServiceActivator::CreateForType<TInterface>();
                return RegistryToBuilder(m_Target);
            }
        };
    } // namespace Internal


    class ServiceRegistry;


    class ServiceRegistryBuilder final
    {
        Rc<ServiceRegistry> m_pRegistry;

        Internal::ServiceRegistrationSpec BindImpl(const UUID& id);

    public:
        ServiceRegistryBuilder(ServiceRegistry* pRegistry);

        void Build();

        template<class TInterface, class = std::enable_if_t<!std::is_base_of_v<ServiceLocatorObjectMarker, TInterface>>>
        inline Internal::RegistryBindBuilder<TInterface> Bind()
        {
            return BindImpl(fe_typeid<TInterface>());
        }
    };
} // namespace FE::DI
