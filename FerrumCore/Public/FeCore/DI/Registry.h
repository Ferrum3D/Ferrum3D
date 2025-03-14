#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/DI/Activator.h>
#include <FeCore/DI/Registration.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/Threading/SpinLock.h>
#include <festd/intrusive_list.h>
#include <festd/vector.h>

namespace FE::DI
{
    struct ServiceRegistry;
    struct LifetimeScope;

    struct ServiceRegistryCallback
    {
        virtual ~ServiceRegistryCallback() = default;
        virtual void OnDetach(ServiceRegistry* pRegistry) = 0;
    };


    struct ServiceRegistry final
        : public Memory::RefCountedObjectBase
        , public festd::intrusive_list_node
    {
        ServiceRegistry(std::pmr::memory_resource* allocator)
            : m_ids(allocator)
            , m_activators(allocator)
            , m_registrations(allocator)
            , m_callbacks(allocator)
        {
        }

        void RegisterCallback(ServiceRegistryCallback* callback)
        {
            std::lock_guard lk{ m_callbackListLock };
            m_callbacks.push_back(callback);
        }

        ~ServiceRegistry() override
        {
            for (ServiceRegistryCallback* callback : m_callbacks)
            {
                callback->OnDetach(this);
            }

            for (const ServiceRegistration& registration : m_registrations)
            {
                if (registration.IsConstant())
                {
                    Memory::RefCountedObjectBase* object;

                    const ResultCode resultCode = m_activators[registration.GetIndex()].Invoke(nullptr, &object);
                    FE_Assert(resultCode == ResultCode::kSuccess);

                    object->Release();
                }
            }
        }

        [[nodiscard]] LifetimeScope* GetRootLifetimeScope() const
        {
            return m_rootLifetimeScope;
        }

        ServiceRegistration* Add(const UUID id)
        {
            const uint32_t index = m_registrations.size();
            m_ids.push_back(id);
            m_activators.push_back({});
            m_registrations.push_back(index);
            return &m_registrations.back();
        }

        void Sort()
        {
            FE_PROFILER_ZONE();
            festd::sort(m_registrations, [this](const ServiceRegistration& lhs, const ServiceRegistration& rhs) {
                return m_ids[lhs.m_index] < m_ids[rhs.m_index];
            });
        }

        ServiceRegistration* FindByID(const UUID id)
        {
            ServiceRegistration* it =
                festd::lower_bound(m_registrations, id, [this](const ServiceRegistration& lhs, const UUID& id) {
                    return m_ids[lhs.m_index] < id;
                });

            if (it != m_registrations.end() && m_ids[it->m_index] == id)
                return it;

            return nullptr;
        }

        ServiceActivator* GetActivator(const uint32_t index)
        {
            return &m_activators[index];
        }

        [[nodiscard]] bool Empty() const
        {
            return m_registrations.empty();
        }

    private:
        friend struct LifetimeScope;
        friend struct ServiceRegistryRoot;

        SegmentedVector<UUID, 2048> m_ids = nullptr;
        SegmentedVector<ServiceActivator, 16384> m_activators = nullptr;
        festd::pmr::vector<ServiceRegistration> m_registrations;
        festd::pmr::vector<ServiceRegistryCallback*> m_callbacks;
        LifetimeScope* m_rootLifetimeScope = nullptr;
        Threading::SpinLock m_callbackListLock;
    };


    struct ServiceRegistryRoot final
    {
        struct Reader final
        {
            explicit Reader(ServiceRegistryRoot* pParent)
                : m_parent(pParent)
            {
                m_parent->m_lock.lock();
            }

            ~Reader()
            {
                m_parent->m_lock.unlock();
            }

            Reader(const Reader&) = delete;
            Reader& operator=(const Reader&) = delete;
            Reader(Reader&&) = delete;
            Reader& operator=(Reader&&) = delete;

            [[nodiscard]] auto begin() const
            {
                return m_parent->m_registries.begin();
            }

            [[nodiscard]] auto end() const
            {
                return m_parent->m_registries.end();
            }

        private:
            ServiceRegistryRoot* m_parent;
        };

        void Initialize()
        {
            FE_PROFILER_ZONE();
            std::pmr::memory_resource* pAllocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kLinear);
            m_root = Rc<ServiceRegistry>::DefaultNew(pAllocator);
            m_registries.push_back(*m_root);
            m_root->RegisterCallback(&m_registryCallback);
        }

        ServiceRegistryRoot()
        {
            m_registryCallback.m_parent = this;
        }

        ~ServiceRegistryRoot()
        {
            m_root.Reset();

            // All references to all the registries must be eliminated at this point.
            FE_Assert(m_registries.empty(), "Service registry leaks detected");
        }

        Reader Read()
        {
            return Reader{ this };
        }

        ServiceRegistry* GetRootRegistry() const
        {
            return m_root.Get();
        }

        ServiceRegistry* Create()
        {
            FE_PROFILER_ZONE();
            std::unique_lock lk{ m_lock };
            ServiceRegistry* pResult = Rc<ServiceRegistry>::DefaultNew(std::pmr::get_default_resource());
            m_registries.push_back(*pResult);
            pResult->RegisterCallback(&m_registryCallback);
            return pResult;
        }

    private:
        festd::intrusive_list<ServiceRegistry> m_registries;
        Rc<ServiceRegistry> m_root;
        Threading::SpinLock m_lock;

        struct CallbackImpl final : public ServiceRegistryCallback
        {
            void OnDetach(ServiceRegistry* pRegistry) override
            {
                std::unique_lock lk{ m_parent->m_lock };
                festd::intrusive_list<ServiceRegistry>::remove(*pRegistry);
            }

            ServiceRegistryRoot* m_parent = nullptr;
        } m_registryCallback;
    };
} // namespace FE::DI
