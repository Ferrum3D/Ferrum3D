#pragma once
#include <FeCore/DI/Builder.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <FeCore/Strings/StringSlice.h>
#include <festd/unordered_map.h>

namespace FE
{
    struct IModule;

    using CreateModuleInstanceProc = void (*)(Env::Internal::IEnvironment&, IModule**);


    //! @brief Base interface for modules and applications.
    struct IModule
    {
        FE_RTTI_Class(IModule, "E7D01BAE-B9D8-4865-A277-27D3CFEC7E39");

        virtual ~IModule() = default;

    protected:
        virtual void RegisterServices(DI::ServiceRegistryBuilder builder) = 0;
    };


    struct ModuleRegistry final : public ServiceLocatorImplBase<Memory::RefCountedObjectBase, ModuleRegistry>
    {
        ModuleRegistry()
        {
            m_allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::Linear);
        }

        IModule* Load(Env::Name moduleName, StringSlice modulePath)
        {
            std::lock_guard lk{ m_lock };
            const auto iter = m_entryMap.find(moduleName);
            if (iter != m_entryMap.end())
            {
                ++iter->second->m_refCount;
                return iter->second->m_module;
            }

            Entry* pNewEntry = AllocateEntryNoLock();
            m_entryMap[moduleName] = pNewEntry;
            pNewEntry->m_name = moduleName;
            pNewEntry->m_refCount = 1;

            const bool loadResult = pNewEntry->m_loader.Load(modulePath);
            FE_AssertMsg(loadResult, "Unable to load module \"{}\"", modulePath);

            const auto createModule = pNewEntry->m_loader.FindFunction<CreateModuleInstanceProc>("CreateModuleInstance");
            FE_AssertMsg(createModule, "Invalid module \"{}\": CreateModuleInstance not found", modulePath);
            createModule(Env::GetEnvironment(), &pNewEntry->m_module);
            return pNewEntry->m_module;
        }

        void Unload(Env::Name moduleName)
        {
            std::lock_guard lk{ m_lock };
            const auto iter = m_entryMap.find(moduleName);
            FE_Assert(iter != m_entryMap.end());
            if (--iter->second->m_refCount == 0)
            {
                Memory::DefaultDelete(iter->second->m_module);
                iter->second->~Entry();
                *reinterpret_cast<Entry**>(&iter->second) = m_freeListHead;
                m_freeListHead = iter->second;
                m_entryMap.erase(iter);
            }
        }

        ~ModuleRegistry()
        {
            for (auto& [name, pEntry] : m_entryMap)
            {
                Trace::ReportError("Module \"{}\" was never unloaded", name);
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
        std::pmr::memory_resource* m_allocator = nullptr;
        Entry* m_freeListHead = nullptr;

        Entry* AllocateEntryNoLock()
        {
            if (m_freeListHead)
            {
                Entry* pResult = m_freeListHead;
                m_freeListHead = *reinterpret_cast<Entry**>(m_freeListHead);
                return pResult;
            }

            return Memory::New<Entry>(m_allocator);
        }
    };


    //! @brief Scoped module loader.
    template<class TModule>
    struct ModuleDependency final
    {
        ModuleDependency()
        {
            m_name = Env::Name{ fe_nameof<TModule>() };
            ModuleRegistry* pModuleRegistry = ServiceLocator<ModuleRegistry>::Get();
            IModule* pModule = pModuleRegistry->Load(m_name, TModule::LibraryPath);

            // TODO: we can't use assert_cast since it doesn't traverse the whole hierarchy.
            m_module = static_cast<TModule*>(pModule);
        }

        ~ModuleDependency()
        {
            ServiceLocator<ModuleRegistry>::Get()->Unload(m_name);
        }

    private:
        TModule* m_module = nullptr;
        Env::Name m_name;

        ModuleDependency(const ModuleDependency&) = delete;
        ModuleDependency& operator=(const ModuleDependency&) = delete;
        ModuleDependency(ModuleDependency&&) = delete;
        ModuleDependency& operator=(ModuleDependency&&) = delete;
    };
} // namespace FE
