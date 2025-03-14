#pragma once
#include <FeCore/IO/BaseIO.h>
#include <FeCore/IO/Path.h>

namespace FE::Platform
{
    IO::Path GetCurrentDirectory();
    void SetCurrentDirectory(festd::string_view path);

    IO::Path GetExecutablePath();


    struct DirectoryIterationParams final
    {
        festd::string_view m_path;
        festd::string_view m_pattern;
        uintptr_t m_callbackData = 0;
        bool (*m_callback)(uintptr_t callbackData, const IO::DirectoryEntry& directoryEntry) = nullptr;
    };

    IO::ResultCode IterateDirectoryRecursively(const DirectoryIterationParams& params);
} // namespace FE::Platform
