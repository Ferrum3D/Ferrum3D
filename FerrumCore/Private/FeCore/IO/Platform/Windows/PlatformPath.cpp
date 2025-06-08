#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/IO/Platform/PlatformPath.h>
#include <FeCore/Platform/Windows/Common.h>

namespace FE::Platform
{
    namespace
    {
        IO::Path MakePlatformPreferred(const festd::string_view path)
        {
            IO::Path result{ path };
            for (uint32_t i = 0; i < result.size(); ++i)
            {
                if (result.byte_at(i) == '/')
                    *(result.data() + i) = '\\';
            }

            return result;
        }
    } // namespace


    IO::Path GetCurrentDirectory()
    {
        FE_PROFILER_ZONE();

        WCHAR buffer[IO::kMaxPathLength + 1];
        const DWORD pathLength = ::GetCurrentDirectoryW(IO::kMaxPathLength, buffer);
        return ConvertWideString<IO::Path>({ buffer, pathLength });
    }


    void SetCurrentDirectory(const festd::string_view path)
    {
        FE_PROFILER_ZONE();

        const WideString widePath{ path };
        ::SetCurrentDirectoryW(widePath.data());
    }


    IO::Path GetExecutablePath()
    {
        FE_PROFILER_ZONE();

        DWORD pathLength = 0;
        WCHAR buffer[IO::kMaxPathLength + 1];
        FE_Verify(::QueryFullProcessImageNameW(::GetCurrentProcess(), 0, buffer, &pathLength));
        return ConvertWideString<IO::Path>({ buffer, pathLength });
    }


    IO::ResultCode IterateDirectoryRecursively(const DirectoryIterationParams& params)
    {
        festd::small_vector<IO::Path, 4> directoryStack;
        directoryStack.push_back(params.m_path);

        while (!directoryStack.empty())
        {
            const IO::Path pattern = MakePlatformPreferred(directoryStack.back() / params.m_pattern);
            const WideString widePattern{ pattern };

            directoryStack.pop_back();

            WIN32_FIND_DATAW findFileData;
            const HANDLE hFile = FindFirstFileW(widePattern.data(), &findFileData);
            if (hFile == INVALID_HANDLE_VALUE)
                return ConvertWin32IOError(GetLastError());

            const auto deferClose = festd::defer([hFile] {
                FindClose(hFile);
            });

            do
            {
                const IO::Path filename = ConvertWideString<IO::Path>(findFileData.cFileName);
                if (filename == "." || filename == "..")
                    continue;

                const IO::Path fullFilename = params.m_path / filename;
                const IO::FileAttributeFlags attributeFlags = ConvertFileAttributeFlags(findFileData.dwFileAttributes);
                const FILETIME creationFT = findFileData.ftCreationTime;
                const FILETIME accessFT = findFileData.ftLastAccessTime;
                const FILETIME writeFT = findFileData.ftLastWriteTime;

                IO::DirectoryEntry entry;
                entry.m_path = IO::PathView{ fullFilename };
                entry.m_attributes = attributeFlags;
                entry.m_stats.m_byteSize = static_cast<uint64_t>(findFileData.nFileSizeHigh) << 32 | findFileData.nFileSizeLow;
                entry.m_stats.m_creationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(creationFT));
                entry.m_stats.m_accessTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(accessFT));
                entry.m_stats.m_modificationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(writeFT));
                if (!params.m_callback(params.m_callbackData, entry))
                    return IO::ResultCode::kCanceled;

                if (Bit::AllSet(attributeFlags, IO::FileAttributeFlags::kDirectory))
                    directoryStack.push_back(fullFilename);
            }
            while (FindNextFileW(hFile, &findFileData));

            if (const DWORD lastError = GetLastError(); lastError != ERROR_NO_MORE_FILES)
                return ConvertWin32IOError(lastError);
        }

        return IO::ResultCode::kSuccess;
    }
} // namespace FE::Platform
