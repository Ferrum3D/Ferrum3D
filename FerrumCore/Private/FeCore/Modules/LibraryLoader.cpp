#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Containers/SmallVector.h>
#include <FeCore/Modules/LibraryLoader.h>

namespace FE
{
    namespace Platform
    {
        ModuleHandle LoadModule(StringSlice name)
        {
#if FE_WINDOWS
            const int32_t nameSize = static_cast<int32_t>(name.Size());
            const int32_t wideLength = MultiByteToWideChar(CP_UTF8, 0, name.Data(), nameSize, nullptr, 0);
            FE_Assert(wideLength > 0);

            festd::small_vector<WCHAR, MAX_PATH + 1> wideName;
            wideName.resize(wideLength + 1, '\0');
            MultiByteToWideChar(CP_UTF8, 0, name.Data(), nameSize, wideName.data(), wideLength);

            const HMODULE hLibrary = LoadLibraryW(wideName.data());
            return { reinterpret_cast<uintptr_t>(hLibrary) };
#else
#    error Not implemented :(
#endif
        }


        bool UnloadModule(ModuleHandle moduleHandle)
        {
#if FE_WINDOWS
            return FreeLibrary(reinterpret_cast<HMODULE>(moduleHandle.Value));
#else
#    error Not implemented :(
#endif
        }


        void* FindModuleSymbol(ModuleHandle moduleHandle, const char* symbolName)
        {
#if FE_WINDOWS
            return (void*)GetProcAddress(reinterpret_cast<HMODULE>(moduleHandle.Value), symbolName);
#else
#    error Not implemented :(
#endif
        }
    } // namespace Platform


    void LibraryLoader::Release()
    {
        if (!m_Handle)
            return;

        const bool result = Platform::UnloadModule(m_Handle);
        FE_AssertMsg(result, "Couldn't unload library");
    }


    LibraryLoader::LibraryLoader(StringSlice libraryName)
    {
        const bool result = Load(libraryName);
        FE_AssertMsg(result, "Couldn't load library \"{}\"", libraryName);
    }
} // namespace FE
