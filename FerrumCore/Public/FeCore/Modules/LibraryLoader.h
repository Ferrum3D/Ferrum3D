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


    struct LibraryLoader final
    {
        LibraryLoader() = default;
        LibraryLoader(StringSlice libraryName);

        bool Load(StringSlice libraryName)
        {
            m_handle = Platform::LoadModule(libraryName);
            return m_handle.IsValid();
        }

        LibraryLoader(LibraryLoader&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle.Reset();
        }

        LibraryLoader& operator=(LibraryLoader&& other) noexcept
        {
            Release();

            m_handle = other.m_handle;
            other.m_handle.Reset();

            return *this;
        }

        Platform::ModuleHandle GetHandle() const
        {
            return m_handle;
        }

        ~LibraryLoader()
        {
            Release();
        }

        template<class T>
        [[nodiscard]] T FindFunction(const char* functionName) const
        {
            return reinterpret_cast<T>(Platform::FindModuleSymbol(m_handle, functionName));
        }

        explicit operator bool() const noexcept
        {
            return m_handle.IsValid();
        }

    private:
        Platform::ModuleHandle m_handle;

        LibraryLoader(const LibraryLoader&) = delete;
        LibraryLoader& operator=(const LibraryLoader&) = delete;

        void Release();
    };
} // namespace FE
