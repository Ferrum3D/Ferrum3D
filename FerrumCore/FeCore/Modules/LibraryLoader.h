#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    namespace Platform
    {
        struct ModuleHandle : TypedHandle<ModuleHandle, uintptr_t, 0>
        {
        };


        ModuleHandle LoadModule(StringSlice fullPath);
        bool UnloadModule(ModuleHandle moduleHandle);

        void* FindModuleSymbol(ModuleHandle moduleHandle, const char* symbolName);
    } // namespace Platform


    class LibraryLoader final
    {
        Platform::ModuleHandle m_Handle;

        LibraryLoader(const LibraryLoader&) = delete;
        LibraryLoader& operator=(const LibraryLoader&) = delete;

        void Release();

    public:
        LibraryLoader() = default;
        LibraryLoader(StringSlice libraryName);

        inline bool Load(StringSlice libraryName)
        {
            m_Handle = Platform::LoadModule(libraryName);
            return m_Handle.IsValid();
        }

        inline LibraryLoader(LibraryLoader&& other) noexcept
        {
            m_Handle = other.m_Handle;
            other.m_Handle.Reset();
        }

        inline LibraryLoader& operator=(LibraryLoader&& other) noexcept
        {
            Release();

            m_Handle = other.m_Handle;
            other.m_Handle.Reset();

            return *this;
        }

        inline Platform::ModuleHandle GetHandle() const
        {
            return m_Handle;
        }

        inline ~LibraryLoader()
        {
            Release();
        }

        template<class T>
        [[nodiscard]] inline T FindFunction(const char* functionName) const
        {
            return reinterpret_cast<T>(Platform::FindModuleSymbol(m_Handle, functionName));
        }

        inline explicit operator bool() const noexcept
        {
            return m_Handle.IsValid();
        }
    };
} // namespace FE
