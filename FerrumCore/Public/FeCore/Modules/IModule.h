#pragma once
#include <FeCore/DI/Builder.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <festd/string.h>
#include <festd/unordered_map.h>

namespace FE
{
    struct IModule;

    using CreateModuleInstanceProc = void (*)(Env::Internal::IEnvironment&, IModule**);


    //! @brief Base interface for dynamic modules.
    struct IModule
    {
        FE_RTTI_Class(IModule, "E7D01BAE-B9D8-4865-A277-27D3CFEC7E39");

        virtual ~IModule() = default;

        virtual void Initialize() = 0;
    };


    struct ModuleRegistry final : public ServiceLocatorImplBase<Memory::RefCountedObjectBase, ModuleRegistry>
    {
        ModuleRegistry() = default;

        ModuleRegistry(const ModuleRegistry&) = delete;
        ModuleRegistry& operator=(const ModuleRegistry&) = delete;
        ModuleRegistry(ModuleRegistry&&) = delete;
        ModuleRegistry& operator=(ModuleRegistry&&) = delete;

        IModule* Load(const Env::Name moduleName, const festd::string_view modulePath)
        {
            std::lock_guard lk{ m_lock };
            const auto iter = m_entryMap.find(moduleName);
            if (iter != m_entryMap.end())
            {
                ++iter->second->m_refCount;
                return iter->second->m_module;
            }

            Entry* pNewEntry = m_entryPool.New();
            m_entryMap[moduleName] = pNewEntry;
            pNewEntry->m_name = moduleName;
            pNewEntry->m_refCount = 1;

            DI::IServiceProvider* serviceProvider = Env::GetServiceProvider();
            Logger* logger = serviceProvider->ResolveRequired<Logger>();

            if (const bool loadResult = pNewEntry->m_loader.Load(modulePath); !loadResult)
            {
                logger->LogError("Unable to load module \"{}\"", modulePath);
                return nullptr;
            }

            const auto createModule = pNewEntry->m_loader.FindFunction<CreateModuleInstanceProc>("CreateModuleInstance");
            if (createModule == nullptr)
            {
                logger->LogError("Unable to find CreateModuleInstance in module \"{}\"", modulePath);
                return nullptr;
            }

            createModule(Env::GetEnvironment(), &pNewEntry->m_module);
            pNewEntry->m_module->Initialize();
            return pNewEntry->m_module;
        }

        void Unload(const Env::Name moduleName)
        {
            std::lock_guard lk{ m_lock };
            const auto iter = m_entryMap.find(moduleName);
            FE_Assert(iter != m_entryMap.end());
            if (--iter->second->m_refCount == 0)
            {
                m_entryPool.Delete(iter->second);
                m_entryMap.erase(iter);
            }
        }

        ~ModuleRegistry() override
        {
            for (auto& [name, pEntry] : m_entryMap)
            {
                FE_AssertMsg(false, "Module \"{}\" was never unloaded", name);
            }
        }

    private:
        struct Entry final
        {
            IModule* m_module = nullptr;
            LibraryLoader m_loader;
            Env::Name m_name;
            uint32_t m_refCount;
        };

        Threading::SpinLock m_lock;
        festd::unordered_dense_map<Env::Name, Entry*> m_entryMap;
        Memory::Pool<Entry> m_entryPool{ "ModuleRegistryPool" };
    };


    struct ModuleLoadingList final
    {
        ModuleLoadingList()
        {
            m_moduleRegistry = ServiceLocator<ModuleRegistry>::Get();
        }

        ~ModuleLoadingList()
        {
            if (m_moduleRegistry)
                Shutdown();
        }

        ModuleLoadingList(const ModuleLoadingList&) = delete;
        ModuleLoadingList& operator=(const ModuleLoadingList&) = delete;
        ModuleLoadingList(ModuleLoadingList&&) = delete;
        ModuleLoadingList& operator=(ModuleLoadingList&&) = delete;

        template<class TModule>
        bool Add()
        {
            const Env::Name moduleName{ fe_nameof<TModule>() };
            if (festd::find_index(m_modules, moduleName) != kInvalidIndex)
                return true;

            if (m_moduleRegistry->Load(moduleName, TModule::LibraryPath))
            {
                m_modules.push_back(moduleName);
                return true;
            }

            return false;
        }

        void Shutdown()
        {
            while (!m_modules.empty())
            {
                m_moduleRegistry->Unload(m_modules.back());
                m_modules.pop_back();
            }

            m_moduleRegistry = nullptr;
        }

    private:
        ModuleRegistry* m_moduleRegistry = nullptr;
        festd::small_vector<Env::Name> m_modules;
    };
} // namespace FE
