#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/IO/Platform/PlatformPath.h>
#include <FeCore/Platform/Windows/Common.h>
#include <FeCore/Strings/Encoding.h>
#include <FeCore/Strings/Utils.h>

namespace FE
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


    IO::Path Platform::GetCurrentDirectory()
    {
        FE_PROFILER_ZONE();

        const DWORD pathLength = ::GetCurrentDirectoryW(0, nullptr);
        festd::inline_vector<WCHAR, MAX_PATH> buffer(pathLength);
        FE_Verify(pathLength == GetCurrentDirectoryW(buffer.size(), buffer.data()) + 1);
        return ConvertWideString<IO::Path>({ buffer.data(), buffer.size() - 1 });
    }


    void Platform::SetCurrentDirectory(const festd::string_view path)
    {
        FE_PROFILER_ZONE();

        const Str::Utf8ToUtf16 widePath{ path.data(), path.size() };
        ::SetCurrentDirectoryW(widePath.ToWideString());
    }


    IO::Path Platform::GetExecutablePath()
    {
        FE_PROFILER_ZONE();

        DWORD pathLength = 0;
        WCHAR buffer[IO::kMaxPathLength + 1];
        FE_Verify(::QueryFullProcessImageNameW(::GetCurrentProcess(), 0, buffer, &pathLength));
        return ConvertWideString<IO::Path>({ buffer, pathLength });
    }


    IO::ResultCode Platform::IterateDirectoryRecursively(const DirectoryIterationParams& params)
    {
        festd::inline_vector<IO::Path, 4> directoryStack;
        directoryStack.push_back(params.m_path);

        const auto patterns = Str::SplitFixed<8>(params.m_pattern, ';');

        while (!directoryStack.empty())
        {
            const IO::Path currentDirectory = directoryStack.back();
            directoryStack.pop_back();

            const IO::Path fullPattern = MakePlatformPreferred(currentDirectory / "*");
            const Str::Utf8ToUtf16 widePattern{ fullPattern.data(), fullPattern.size() };

            WIN32_FIND_DATAW findFileData;
            const HANDLE hFile = FindFirstFileW(widePattern.ToWideString(), &findFileData);
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

                const IO::Path fullFilename = currentDirectory / filename;
                const IO::FileAttributeFlags attributeFlags = ConvertFileAttributeFlags(findFileData.dwFileAttributes);
                const FILETIME creationFT = findFileData.ftCreationTime;
                const FILETIME accessFT = findFileData.ftLastAccessTime;
                const FILETIME writeFT = findFileData.ftLastWriteTime;

                if (!Bit::AnySet(attributeFlags, IO::FileAttributeFlags::kDirectory))
                {
                    if (!Str::MatchAny(filename, patterns))
                        continue;
                }

                IO::DirectoryEntry entry;
                entry.m_path = IO::PathView{ fullFilename };
                entry.m_attributes = attributeFlags;
                entry.m_stats.m_byteSize = static_cast<uint64_t>(findFileData.nFileSizeHigh) << 32 | findFileData.nFileSizeLow;
                entry.m_stats.m_creationTime = ConvertFiletimeToDateTime<TZ::UTC>(creationFT);
                entry.m_stats.m_accessTime = ConvertFiletimeToDateTime<TZ::UTC>(accessFT);
                entry.m_stats.m_modificationTime = ConvertFiletimeToDateTime<TZ::UTC>(writeFT);
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
} // namespace FE
