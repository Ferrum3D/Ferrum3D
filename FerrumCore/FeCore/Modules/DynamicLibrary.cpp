#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Console/FeLog.h>
#include <FeCore/Modules/DynamicLibrary.h>

#if FE_WINDOWS
#    define FE_LOAD_LIBRARY(name) LoadLibraryA(name)
#    define FE_GET_PROC_ADDR(handle, name) reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name))
#    define FE_FREE_LIBRARY(handle) FreeLibrary(static_cast<HMODULE>(handle))
#else
#    define FE_LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
#    define FE_GET_PROC_ADDR(handle, name) dlsym(handle, name)
#    define FE_FREE_LIBRARY(handle) dlclose(handle)
#endif

namespace FE
{
    bool DynamicLibrary::LoadFrom(StringSlice fileName)
    {
        m_FullName = fileName;
        m_FullName += FE_DLL_EXTENSION;
        m_NativeHandle = FE_LOAD_LIBRARY(m_FullName.Data());
        if (!m_NativeHandle)
        {
            m_FullName = fileName;
            m_FullName += "d";
            m_FullName += FE_DLL_EXTENSION;
            m_NativeHandle = FE_LOAD_LIBRARY(m_FullName.Data());
        }

        if (m_NativeHandle)
        {
            FE_LOG_MESSAGE("Loaded a dynamic library from file '{}'", m_FullName);
            return true;
        }
        else
        {
            FE_LOG_ERROR("Library '{}{}' was not loaded due to an error", fileName, FE_DLL_EXTENSION);
            return false;
        }
    }

    void* DynamicLibrary::GetFunctionImpl(StringSlice functionName)
    {
        return FE_GET_PROC_ADDR(m_NativeHandle, functionName.Data());
    }

    DynamicLibrary::~DynamicLibrary()
    {
        Unload();
    }

    void DynamicLibrary::Unload()
    {
        if (m_NativeHandle == nullptr)
        {
            return;
        }

        FE_FREE_LIBRARY(m_NativeHandle);
        FE_LOG_MESSAGE("Unloaded dynamic library from file '{}'", m_FullName);
        m_FullName = "";
        m_NativeHandle = nullptr;
    }
} // namespace FE
