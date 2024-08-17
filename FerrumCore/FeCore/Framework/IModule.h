#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/HashTables.h>
#include <FeCore/DI/Builder.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    class IModule;

    using CreateModuleInstanceProc = void (*)(Env::Internal::IEnvironment&, IModule**);


    //! \brief Base interface for modules and applications.
    class IModule
    {
    protected:
        virtual void RegisterServices(DI::ServiceRegistryBuilder builder) = 0;

    public:
        FE_RTTI_Class(IModule, "E7D01BAE-B9D8-4865-A277-27D3CFEC7E39");

        virtual ~IModule() = default;
    };


    class ModuleRegistry final : public ServiceLocatorImplBase<Memory::RefCountedObjectBase, ModuleRegistry>
    {
        struct Entry final
        {
            IModule* pModule = nullptr;
            LibraryLoader Loader;
            Env::Name Name;
            uint32_t m_RefCount;
        };

        SpinLock m_Lock;
        festd::unordered_dense_map<Env::Name, Entry*> m_EntryMap;
        std::pmr::memory_resource* m_pAllocator = nullptr;
        Entry* m_pFreeListHead = nullptr;

        inline Entry* AllocateEntryNoLock()
        {
            if (m_pFreeListHead)
            {
                Entry* pResult = m_pFreeListHead;
                m_pFreeListHead = *reinterpret_cast<Entry**>(m_pFreeListHead);
                return pResult;
            }

            return Memory::New<Entry>(m_pAllocator);
        }

    public:
        inline ModuleRegistry()
        {
            m_pAllocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
        }

        inline IModule* Load(Env::Name moduleName, StringSlice modulePath)
        {
            std::lock_guard lk{ m_Lock };
            const auto iter = m_EntryMap.find(moduleName);
            if (iter != m_EntryMap.end())
            {
                ++iter->second->m_RefCount;
                return iter->second->pModule;
            }

            Entry* pNewEntry = AllocateEntryNoLock();
            m_EntryMap[moduleName] = pNewEntry;
            pNewEntry->Name = moduleName;
            pNewEntry->m_RefCount = 1;
            pNewEntry->Loader.Load(modulePath);

            const auto createModule = pNewEntry->Loader.FindFunction<CreateModuleInstanceProc>("CreateModuleInstance");
            FE_ASSERT_MSG(createModule, "Invalid module \"{}\": CreateModuleInstance not found");
            createModule(Env::GetEnvironment(), &pNewEntry->pModule);
            return pNewEntry->pModule;
        }

        inline void Unload(Env::Name moduleName)
        {
            std::lock_guard lk{ m_Lock };
            const auto iter = m_EntryMap.find(moduleName);
            FE_ASSERT(iter != m_EntryMap.end());
            if (--iter->second->m_RefCount == 0)
            {
                Memory::DefaultDelete(iter->second->pModule);
                iter->second->~Entry();
                *reinterpret_cast<Entry**>(&iter->second) = m_pFreeListHead;
                m_pFreeListHead = iter->second;
                m_EntryMap.erase(iter);
            }
        }

        inline ~ModuleRegistry()
        {
            for (auto& [name, pEntry] : m_EntryMap)
            {
                FE_LOG_ERROR("Module \"{}\" was never unloaded", name);
            }
        }
    };


    //! \brief Scoped module loader.
    template<class TModule>
    class ModuleDependency final
    {
        TModule* m_pModule = nullptr;
        Env::Name m_Name;

        ModuleDependency(const ModuleDependency&) = delete;
        ModuleDependency& operator=(const ModuleDependency&) = delete;
        ModuleDependency(ModuleDependency&&) = delete;
        ModuleDependency& operator=(ModuleDependency&&) = delete;

    public:
        inline ModuleDependency()
        {
            m_Name = Env::Name{ fe_nameof<TModule>() };
            ModuleRegistry* pModuleRegistry = ServiceLocator<ModuleRegistry>::Get();
            IModule* pModule = pModuleRegistry->Load(m_Name, TModule::LibraryPath);

            // TODO: we can't use assert_cast, because it doesn't traverse the whole hierarchy.
            m_pModule = static_cast<TModule*>(pModule);
        }

        inline ~ModuleDependency()
        {
            ServiceLocator<ModuleRegistry>::Get()->Unload(m_Name);
        }
    };
} // namespace FE
