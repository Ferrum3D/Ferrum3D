#pragma once
#include <FeCore/Framework/FrameworkBase.h>

namespace FE
{
    //! \brief A default generic implementation of IFrameworkFactory for dynamic modules.
    template<class TModule>
    class ModuleFrameworkFactoryImpl : public Object<IFrameworkFactory>
    {
        StringSlice m_LibraryPath;
        Shared<DynamicLibrary> m_Library;
        TModule* m_Module = nullptr;

    public:
        typedef TModule* (*CreateModuleInstanceProc)(Env::Internal::IEnvironment* environment);
        typedef void (*DetachModuleProc)();

        inline ModuleFrameworkFactoryImpl(StringSlice libraryPath) // NOLINT
            : m_LibraryPath(libraryPath)
        {
        }

        inline bool IsLoaded() override
        {
            return SharedInterface<TModule>::Get() != nullptr;
        }

        inline void Load() override
        {
            if (IsLoaded())
            {
                return;
            }

            m_Library = MakeShared<DynamicLibrary>();
            m_Library->LoadFrom(m_LibraryPath);
            auto CreateModuleInstance = m_Library->GetFunction<CreateModuleInstanceProc>("CreateModuleInstance");
            m_Module                  = CreateModuleInstance(&Env::GetEnvironment());
            FE_LOG_MESSAGE("Loaded a dynamic module: {}", fe_nameof<TModule>());
        }

        inline void Unload() override
        {
            m_Module->ReleaseStrongRef();
            m_Library.Reset();
            FE_LOG_MESSAGE("Unloaded a dynamic module: {}", fe_nameof<TModule>());
        }
    };

    struct ModuleInfo
    {
        String Name;
        String Description;
        String Author;

        inline ModuleInfo() = default;

        inline ModuleInfo(const String& name, const String& description, const String& author)
            : Name(name)
            , Description(description)
            , Author(author)
        {
        }
    };

    //! \brief Base class for all modules in Ferrum3D.
    //!
    //! Usage:
    //! \code{.cpp}
    //!     class MyModule : public ModuleFramework<MyModule>
    //!     {
    //!     public:
    //!         // Initialize module:
    //!         void Initialize(const MyModuleDesc& desc) { FrameworkBase::Initialize(); ... }
    //!     };
    //!
    //!     // Implement a function to create MyModule (or you can create a custom factory and use another way):
    //!     extern "C" FE_DLL_EXPORT MyModule* CreateModuleInstance(Env::Internal::IEnvironment* environment) { ... }
    //!
    //!     // To refer to the module instance:
    //!     MyModule& mod = SharedInterface<MyModule>::Get();
    //!
    //!     // To add as a framework dependency in FrameworkBase::GetFrameworkDependencies():
    //!     dependencies.Push(MyModule::CreateFactory());
    //! \endcode
    template<class TModule>
    class ModuleFramework : public FrameworkBase
    {
        ModuleInfo m_Info;

    protected:
        inline void SetInfo(const ModuleInfo& info)
        {
            m_Info = info;
        }

    public:
        FE_CLASS_RTTI(ModuleFramework, "ED879368-A819-4B8C-AF2C-848AEF12B99F");

        ~ModuleFramework() override = default;

        inline ModuleFramework() = default;

        using Factory = ModuleFrameworkFactoryImpl<TModule>;
        inline static Shared<IFrameworkFactory> CreateFactory()
        {
            return static_pointer_cast<IFrameworkFactory>(MakeShared<Factory>(TModule::LibraryPath));
        }

        [[nodiscard]] inline const ModuleInfo& GetInfo() const noexcept
        {
            return m_Info;
        }
    };
} // namespace FE