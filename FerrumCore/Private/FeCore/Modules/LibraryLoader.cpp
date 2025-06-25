#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Modules/LibraryLoader.h>
#include <festd/vector.h>

namespace FE
{
    namespace Platform
    {
        ModuleHandle LoadModule(const festd::string_view name)
        {
            FE_PROFILER_ZONE_TEXT("%.*s", name.size(), name.data());

#if FE_PLATFORM_WINDOWS
            const int32_t nameSize = static_cast<int32_t>(name.size());
            const int32_t wideLength = MultiByteToWideChar(CP_UTF8, 0, name.data(), nameSize, nullptr, 0);
            FE_Assert(wideLength > 0);

            festd::inline_vector<WCHAR, MAX_PATH + 1> wideName;
            wideName.resize(wideLength + 1, '\0');
            MultiByteToWideChar(CP_UTF8, 0, name.data(), nameSize, wideName.data(), wideLength);

            const HMODULE hLibrary = LoadLibraryW(wideName.data());
            return { reinterpret_cast<uintptr_t>(hLibrary) };
#else
#    error Not implemented :(
#endif
        }


        bool UnloadModule(const ModuleHandle moduleHandle)
        {
#if FE_PLATFORM_WINDOWS
            return FreeLibrary(reinterpret_cast<HMODULE>(moduleHandle.m_value));
#else
#    error Not implemented :(
#endif
        }


        void* FindModuleSymbol(const ModuleHandle moduleHandle, const char* symbolName)
        {
#if FE_PLATFORM_WINDOWS
            return (void*)GetProcAddress(reinterpret_cast<HMODULE>(moduleHandle.m_value), symbolName);
#else
#    error Not implemented :(
#endif
        }
    } // namespace Platform


    void LibraryLoader::Release()
    {
        if (!m_handle)
            return;

        const bool result = Platform::UnloadModule(m_handle);
        m_handle = {};
        FE_AssertMsg(result, "Couldn't unload library");
    }


    LibraryLoader::LibraryLoader(const festd::string_view libraryName)
    {
        const bool result = Load(libraryName);
        FE_AssertMsg(result, "Couldn't load library \"{}\"", libraryName);
    }
} // namespace FE
