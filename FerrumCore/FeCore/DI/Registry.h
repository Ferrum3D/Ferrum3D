#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/DI/Activator.h>
#include <FeCore/DI/Registration.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::DI
{
    class ServiceRegistry;

    struct ServiceRegistryCallback : festd::intrusive_list_node
    {
        virtual void OnDetach(ServiceRegistry* pRegistry) = 0;
    };


    class ServiceRegistry final
        : public Memory::RefCountedObjectBase
        , public festd::intrusive_list_node
    {
        friend class LifetimeScope;
        friend class ServiceRegistryRoot;

        SegmentedVector<UUID, 1024> m_IDs = nullptr;
        SegmentedVector<ServiceActivator> m_Activators = nullptr;
        festd::pmr::vector<ServiceRegistration> m_Registrations;
        festd::intrusive_list<ServiceRegistryCallback> m_Callbacks;
        LifetimeScope* m_pRootLifetimeScope = nullptr;
        SpinLock m_CallbackListLock;

        inline ServiceRegistry(std::pmr::memory_resource* pAllocator)
            : m_IDs(pAllocator)
            , m_Activators(pAllocator)
            , m_Registrations(pAllocator)
        {
        }

    public:
        inline void RegisterCallback(ServiceRegistryCallback& callback)
        {
            std::lock_guard lk{ m_CallbackListLock };
            m_Callbacks.push_back(callback);
        }

        inline ~ServiceRegistry() override
        {
            for (ServiceRegistryCallback& callback : m_Callbacks)
            {
                callback.OnDetach(this);
            }

            for (const ServiceRegistration& registration : m_Registrations)
            {
                if (registration.IsConstant())
                {
                    Memory::RefCountedObjectBase* pObject;

                    [[maybe_unused]] const ResultCode resultCode =
                        m_Activators[registration.GetIndex()].Invoke(nullptr, &pObject);
                    FE_CORE_ASSERT(resultCode == ResultCode::Success, "");

                    pObject->Release();
                }
            }
        }

        [[nodiscard]] inline LifetimeScope* GetRootLifetimeScope() const
        {
            return m_pRootLifetimeScope;
        }

        inline ServiceRegistration* Add(const UUID& id)
        {
            const uint32_t index = m_Registrations.size();
            m_IDs.push_back(id);
            m_Activators.push_back({});
            m_Registrations.emplace_back(index);
            return &m_Registrations.back();
        }

        inline void Sort()
        {
            ZoneScoped;
            eastl::sort(m_Registrations.begin(),
                        m_Registrations.end(),
                        [this](const ServiceRegistration& lhs, const ServiceRegistration& rhs) {
                            return m_IDs[lhs.m_Index] < m_IDs[rhs.m_Index];
                        });
        }

        inline ServiceRegistration* FindByID(const UUID& id)
        {
            ServiceRegistration* it = eastl::lower_bound(
                m_Registrations.begin(), m_Registrations.end(), id, [this](const ServiceRegistration& lhs, const UUID& id) {
                    return m_IDs[lhs.m_Index] < id;
                });

            if (it != m_Registrations.end() && m_IDs[it->m_Index] == id)
                return it;

            return nullptr;
        }

        inline ServiceActivator* GetActivator(uint32_t index)
        {
            return &m_Activators[index];
        }
    };


    class ServiceRegistryRoot final
    {
        Memory::PoolAllocator m_RegistryPool;
        festd::intrusive_list<ServiceRegistry> m_Registries;
        Rc<ServiceRegistry> m_pRoot;
        SpinLock m_Lock;

        struct CallbackImpl final : ServiceRegistryCallback
        {
            inline void OnDetach(ServiceRegistry* pRegistry) override
            {
                ServiceRegistryRoot* pParent = reinterpret_cast<ServiceRegistryRoot*>(
                    reinterpret_cast<uintptr_t>(this) - offsetof(ServiceRegistryRoot, m_RegistryCallback));

                std::unique_lock lk{ pParent->m_Lock };
                pParent->m_Registries.remove(*pRegistry);
            }
        } m_RegistryCallback;

    public:
        class Reader final
        {
            ServiceRegistryRoot* m_pParent;

        public:
            inline Reader(ServiceRegistryRoot* pParent)
                : m_pParent(pParent)
            {
                m_pParent->m_Lock.lock();
            }

            inline ~Reader()
            {
                m_pParent->m_Lock.unlock();
            }

            inline auto begin() const
            {
                return m_pParent->m_Registries.begin();
            }

            inline auto end() const
            {
                return m_pParent->m_Registries.end();
            }
        };

        inline ServiceRegistryRoot()
            : m_RegistryPool("ServiceRegistryPool", sizeof(ServiceRegistry), 16 * 1024)
        {
            std::pmr::memory_resource* pAllocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
            m_pRoot = Rc<ServiceRegistry>::New(&m_RegistryPool, pAllocator);
            m_Registries.push_back(*m_pRoot);
            m_pRoot->RegisterCallback(m_RegistryCallback);
        }

        inline ~ServiceRegistryRoot()
        {
            m_pRoot.Reset();

            // All references to all of the registries must be eliminated at this point.
            FE_CORE_ASSERT(m_Registries.empty(), "Service registry leaks detected");
        }

        inline Reader Read()
        {
            return Reader{ this };
        }

        inline ServiceRegistry* GetRootRegistry() const
        {
            return m_pRoot.Get();
        }

        inline ServiceRegistry* Create()
        {
            std::unique_lock lk{ m_Lock };
            ServiceRegistry* pResult = Rc<ServiceRegistry>::New(&m_RegistryPool, std::pmr::get_default_resource());
            m_Registries.push_back(*pResult);
            pResult->RegisterCallback(m_RegistryCallback);
            return pResult;
        }
    };
} // namespace FE::DI
