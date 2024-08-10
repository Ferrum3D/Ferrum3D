#pragma once
#include <FeCore/Console/FeLog.h>
#include <FeCore/Modules/DynamicLibrary.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    class IFramework;

    //! \brief A class that loads, unloads frameworks and holds a DynamicLibrary instance if needed.
    class IFrameworkFactory : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IFrameworkFactory, "4937DF6A-5A41-4881-9C30-123A486C647C");

        //! \brief True if the framework was loaded.
        virtual bool IsLoaded() = 0;

        //! \brief True is the framework was loaded from this factory.
        //!
        //! IFrameworkFactory::Load() only actually loads the module if it was not loaded previously be another instance
        //! of its factory. Multiple frameworks can depend on the same module, but it shouldn't be loaded and unloaded
        //! multiple times. This function indicates that the module won't outlive the framework that owns this factory.
        virtual bool IsOwner() = 0;

        //! \brief Load a framework.
        //!
        //! This function loads the framework to the memory, but doesn't initialize it and doesn't load its dependencies.
        //!
        //! Since all frameworks implement ServiceLocator, it doesn't return anything.
        virtual void Load() = 0;

        //! \brief Unload a framework.
        virtual void Unload() = 0;
    };

    //! \brief Base class for module and application frameworks.
    //!
    //! A framework is a base of any engine components: all modules and the user application.
    //! Any framework can have dependencies which are other frameworks. This creates a dependency tree that is loaded by the
    //! FrameworkBase class. All operations with frameworks must be performed on main thread.
    //!
    //! The framework's lifecycle has the following stages:
    //! 1) Loading: the framework's DLL is loaded through IFrameworkFactory.
    //! 2) Initialization: the framework is initialized by its user (only on the first actual use). The dependencies are loaded
    //!      on this stage.
    //! 3) Unloading: the framework is unloaded through IFrameworkFactory. The DLL and all the dependencies are unloaded here.
    //!      The framework can clean up all its data if needed.
    //!
    //! If the framework is not a dependency of any other framework and is not a DLL, e.g. it is an ApplicationFramework,
    //! it can be created manually.
    class IFramework : public Memory::RefCountedObjectBase
    {
    protected:
        //! \brief Get list of factories for all dependencies.
        //!
        //! \param [out] dependencies - The list of dependencies. New entries must be added to it.
        virtual void GetFrameworkDependencies(eastl::vector<Rc<IFrameworkFactory>>& dependencies) = 0;

        //! \brief Basic framework initialization. Loads dependencies, sets initialized flag to true.
        virtual void Initialize() = 0;

        //! \brief Unloads all dependencies.
        virtual void UnloadDependencies() = 0;

    public:
        FE_RTTI_Class(IFramework, "E7D01BAE-B9D8-4865-A277-27D3CFEC7E39");

        ~IFramework() override = default;

        //! \brief True if the framework is initialized and is ready to use.
        virtual bool IsInitialized() = 0;
    };
} // namespace FE
