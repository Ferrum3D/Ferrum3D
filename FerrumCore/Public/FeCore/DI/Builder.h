#pragma once
#include <FeCore/DI/Activator.h>
#include <FeCore/DI/Registration.h>
#include <FeCore/DI/Registry.h>

namespace FE::DI
{
    struct ServiceRegistryBuilder;


    namespace Internal
    {
        struct ServiceRegistrationSpec final
        {
            ServiceRegistration* m_registration = nullptr;
            ServiceActivator* m_activator = nullptr;
        };


        struct [[nodiscard]] RegistryToBuilder final
        {
            void InScope(const Lifetime lifetime)
            {
                m_target.m_registration->m_lifetime = lifetime;
            }

            void InSingletonScope()
            {
                InScope(Lifetime::kSingleton);
            }

            void InThreadScope()
            {
                InScope(Lifetime::kThread);
            }

            void InTransientScope()
            {
                InScope(Lifetime::kTransient);
            }

        private:
            template<class TInterface>
            friend struct RegistryBindBuilder;

            ServiceRegistrationSpec m_target;

            explicit RegistryToBuilder(ServiceRegistrationSpec registrationSpec)
                : m_target(registrationSpec)
            {
            }
        };


        template<class TInterface>
        struct [[nodiscard]] RegistryBindBuilder final
        {
            template<class TImpl>
            RegistryToBuilder To()
            {
                if constexpr (!std::is_same_v<TImpl, TInterface>)
                {
                    FE_Assert(fe_typeid<TImpl>() != fe_typeid<TInterface>());
                }

                *m_target.m_activator = ServiceActivator::CreateForType<TImpl>();
                return RegistryToBuilder(m_target);
            }

            RegistryToBuilder ToFunc(ActivatorFunction&& function)
            {
                *m_target.m_activator = ServiceActivator::CreateFromFunction(std::forward<ActivatorFunction>(function));
                m_target.m_registration->m_isFunction = true;
                return RegistryToBuilder(m_target);
            }

            void ToConst(TInterface* pConst)
            {
                auto factory = [pConst](IServiceProvider*, Memory::RefCountedObjectBase** ppResult) {
                    *ppResult = pConst;
                    return ResultCode::kSuccess;
                };

                *m_target.m_activator = ServiceActivator::CreateFromFunction(factory);
                m_target.m_registration->m_lifetime = Lifetime::kSingleton;
                m_target.m_registration->m_isConstant = true;
                pConst->AddRef();
            }

            RegistryToBuilder ToSelf()
            {
                *m_target.m_activator = ServiceActivator::CreateForType<TInterface>();
                return RegistryToBuilder(m_target);
            }

        private:
            static_assert(std::is_base_of_v<Memory::RefCountedObjectBase, TInterface>);

            friend struct ServiceRegistryBuilder;
            ServiceRegistrationSpec m_target;

            RegistryBindBuilder(const ServiceRegistrationSpec registrationSpec)
                : m_target(registrationSpec)
            {
            }
        };
    } // namespace Internal


    struct ServiceRegistry;


    struct ServiceRegistryBuilder final
    {
        ServiceRegistryBuilder(ServiceRegistry* pRegistry);

        void Build();

        template<class TInterface>
        Internal::RegistryBindBuilder<TInterface> Bind() const
        {
            return BindImpl(fe_typeid<TInterface>());
        }

    private:
        Rc<ServiceRegistry> m_registry;

        Internal::ServiceRegistrationSpec BindImpl(const UUID& id) const;
    };


    void RegisterCoreServices(const ServiceRegistryBuilder& builder);
} // namespace FE::DI
