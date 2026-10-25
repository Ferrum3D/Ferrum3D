#include <Core/Base/PlatformInclude.h>
#include <Core/IO/DirectoryWatcher.h>
#include <Core/Logging/Trace.h>
#include <Core/Platform/Windows/Common.h>
#include <Core/Strings/Encoding.h>

namespace FE::IO
{
    namespace
    {
        constexpr DWORD kNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;

        //! ReadDirectoryChangesW requires a DWORD-aligned buffer.
        constexpr uint32_t kBufferDwordCount = 16 * 1024;


        struct WindowsDirectoryWatcher final : public DirectoryWatcher
        {
            WindowsDirectoryWatcher(Path root, const HANDLE directory, const HANDLE event)
                : m_root(std::move(root))
                , m_directory(directory)
            {
                m_overlapped.hEvent = event;
                m_buffer.resize(kBufferDwordCount);
            }

            ~WindowsDirectoryWatcher() override
            {
                if (m_isPending)
                {
                    DWORD bytesTransferred = 0;
                    CancelIoEx(m_directory, &m_overlapped);
                    GetOverlappedResult(m_directory, &m_overlapped, &bytesTransferred, TRUE);
                }

                CloseHandle(m_directory);
                CloseHandle(m_overlapped.hEvent);
            }

            bool IssueRead()
            {
                ResetEvent(m_overlapped.hEvent);
                m_isPending = ReadDirectoryChangesW(m_directory,
                                                    m_buffer.data(),
                                                    m_buffer.size() * sizeof(DWORD),
                                                    TRUE,
                                                    kNotifyFilter,
                                                    nullptr,
                                                    &m_overlapped,
                                                    nullptr);
                if (!m_isPending)
                    Logger::LogError("Failed to watch directory '{}': {}", m_root, GetLastError());

                return m_isPending;
            }

            bool PollChanges(festd::vector<Path>& changedFiles) override
            {
                FE_PROFILER_ZONE();

                if (!m_isPending)
                    return true;

                DWORD bytesTransferred = 0;
                if (!GetOverlappedResult(m_directory, &m_overlapped, &bytesTransferred, FALSE))
                {
                    if (GetLastError() == ERROR_IO_INCOMPLETE)
                        return true;

                    Logger::LogError("Stopped watching directory '{}': {}", m_root, GetLastError());
                    m_isPending = false;
                    return false;
                }

                // Zero bytes means that the notifications did not fit into the buffer and were dropped.
                const bool isComplete = bytesTransferred != 0;
                if (isComplete)
                    AppendChanges(changedFiles);

                IssueRead();
                return isComplete;
            }

        private:
            void AppendChanges(festd::vector<Path>& changedFiles) const
            {
                const auto* bytes = reinterpret_cast<const std::byte*>(m_buffer.data());
                for (;;)
                {
                    const auto* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(bytes);
                    const uint32_t nameLength = info->FileNameLength / sizeof(WCHAR);

                    Path relativePath = Platform::ConvertWideString<Path>(festd::span{ info->FileName, nameLength });
                    for (uint32_t index = 0; index < relativePath.size(); ++index)
                    {
                        if (relativePath.byte_at(index) == '\\')
                            *(relativePath.data() + index) = '/';
                    }

                    changedFiles.push_back(m_root / relativePath);

                    if (info->NextEntryOffset == 0)
                        break;

                    bytes += info->NextEntryOffset;
                }
            }

            void DoRelease() override
            {
                Memory::DefaultDelete(this);
            }

            Path m_root;
            HANDLE m_directory = INVALID_HANDLE_VALUE;
            OVERLAPPED m_overlapped{};
            festd::vector<DWORD> m_buffer;
            bool m_isPending = false;
        };
    } // namespace


    Rc<DirectoryWatcher> DirectoryWatcher::Create(const festd::string_view directory)
    {
        FE_PROFILER_ZONE();

        const Path root = NormalizePath(directory);
        const Str::Utf8ToUtf16 widePath{ root.data(), root.size() };
        const HANDLE directoryHandle = CreateFileW(widePath.ToWideString(),
                                                   FILE_LIST_DIRECTORY,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                   nullptr,
                                                   OPEN_EXISTING,
                                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                                   nullptr);
        if (directoryHandle == INVALID_HANDLE_VALUE)
            return nullptr;

        const HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (event == nullptr)
        {
            CloseHandle(directoryHandle);
            return nullptr;
        }

        Rc watcher = Memory::DefaultNew<WindowsDirectoryWatcher>(root, directoryHandle, event);
        if (!watcher->IssueRead())
            return nullptr;

        return watcher;
    }
} // namespace FE::IO
