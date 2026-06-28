#include <Core/IO/AsyncStreamIO.h>
#include <Core/IO/Platform/Windows/OverlappedAsyncIOBackend.h>
#include <Core/Memory/Memory.h>
#include <Core/Platform/Windows/Common.h>
#include <Core/Strings/Encoding.h>

namespace FE::IO
{
    namespace
    {
        ResultCode ConvertWin32OverlappedIOError(const DWORD error)
        {
            switch (error)
            {
            case ERROR_OPERATION_ABORTED:
                return ResultCode::kCanceled;
            case ERROR_HANDLE_EOF:
                return ResultCode::kIOError;
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return ResultCode::kNoFileOrDirectory;
            case ERROR_ACCESS_DENIED:
                return ResultCode::kPermissionDenied;
            case ERROR_SHARING_VIOLATION:
            case ERROR_INVALID_PARAMETER:
                return ResultCode::kInvalidArgument;
            case ERROR_FILE_TOO_LARGE:
                return ResultCode::kFileTooLarge;
            case ERROR_TOO_MANY_OPEN_FILES:
                return ResultCode::kTooManyOpenFiles;
            case ERROR_SEEK:
                return ResultCode::kInvalidSeek;
            case ERROR_NOT_SUPPORTED:
                return ResultCode::kNotSupported;
            default:
                return ResultCode::kIOError;
            }
        }
    } // namespace


    OverlappedAsyncIOBackend::OverlappedAsyncIOBackend()
    {
        m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        m_iocpWaitThread.Start("IOCP Wait Thread", [this] {
            for (;;)
            {
            }
        });
    }


    OverlappedAsyncIOBackend::~OverlappedAsyncIOBackend()
    {
        for (PendingRead* read : m_reads)
            CloseRead(read);

        for (PendingRead* read : m_queuedReads)
            CloseRead(read);

        if (m_completionPort)
            CloseHandle(m_completionPort);
    }


    festd::expected<Platform::FileHandle, ResultCode> OverlappedAsyncIOBackend::OpenFile(const festd::string_view filePath)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", filePath.size(), filePath.data());

        const Str::Utf8ToUtf16 widePath{ filePath.data(), filePath.size() };
        const HANDLE nativeFileHandle = CreateFileW(widePath.ToWideString(),
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ,
                                                    nullptr,
                                                    OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                                    nullptr);

        if (nativeFileHandle == INVALID_HANDLE_VALUE)
            return festd::unexpected(Platform::ConvertWin32IOError(GetLastError()));

        return Platform::FileHandle::FromPointer(nativeFileHandle);
    }


    AsyncReadHandle OverlappedAsyncIOBackend::DispatchRead(const AsyncIOPhysicalRead& read)
    {
        const Platform::FileHandle fileHandle = read.m_file->Get();

        auto* pendingRead = Memory::DefaultNew<PendingRead>();
        pendingRead->m_file = reinterpret_cast<HANDLE>(fileHandle.m_value);
        pendingRead->m_destination = read.m_destination;
        pendingRead->m_size = read.m_size;
        pendingRead->m_group = read.m_group;
        pendingRead->m_handle = AsyncReadHandle{ m_nextHandle++ };
        pendingRead->m_overlapped.Offset = static_cast<DWORD>(read.m_offset & 0xffffffffu);
        pendingRead->m_overlapped.OffsetHigh = static_cast<DWORD>((read.m_offset >> 32) & 0xffffffffu);

        if (m_outstandingReadCount >= kMaxOutstandingReads)
        {
            m_queuedReads.push_back(pendingRead);
            return pendingRead->m_handle;
        }

        return StartRead(pendingRead);
    }


    AsyncReadHandle OverlappedAsyncIOBackend::StartRead(PendingRead* read)
    {
        m_reads.push_back(read);
        read->m_started = true;
        ++m_outstandingReadCount;

        if (read->m_file == INVALID_HANDLE_VALUE)
        {
            PostQueuedCompletionStatus(m_completionPort, 0, static_cast<ULONG_PTR>(read->m_handle.m_value), &read->m_overlapped);
            return read->m_handle;
        }

        CreateIoCompletionPort(read->m_file, m_completionPort, static_cast<ULONG_PTR>(read->m_handle.m_value), 0);

        const size_t bytesToRead = Math::Min<size_t>(read->m_size, Constants::kMaxValue<DWORD>);
        DWORD bytesRead = 0;
        if (!ReadFile(read->m_file, read->m_destination, static_cast<DWORD>(bytesToRead), &bytesRead, &read->m_overlapped))
        {
            const DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
            {
                PostQueuedCompletionStatus(m_completionPort,
                                           0,
                                           static_cast<ULONG_PTR>(read->m_handle.m_value),
                                           &read->m_overlapped);
            }
        }

        return read->m_handle;
    }


    bool OverlappedAsyncIOBackend::PollRequestCompletion(AsyncIOCompletion& completion)
    {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;
        if (!GetQueuedCompletionStatus(m_completionPort, &bytesTransferred, &completionKey, &overlapped, 0))
        {
            const DWORD error = GetLastError();
            if (overlapped == nullptr)
                return false;

            completion.m_result = ConvertWin32OverlappedIOError(error);
        }
        else
        {
            completion.m_result = ResultCode::kSuccess;
        }

        PendingRead* read = nullptr;
        for (uint32_t index = 0; index < m_reads.size(); ++index)
        {
            if (&m_reads[index]->m_overlapped == overlapped)
            {
                read = m_reads[index];
                m_reads.erase(m_reads.begin() + index);
                break;
            }
        }

        if (read == nullptr)
            return false;

        if (read->m_file == INVALID_HANDLE_VALUE)
            completion.m_result = ConvertWin32OverlappedIOError(ERROR_FILE_NOT_FOUND);

        completion.m_handle = read->m_handle;
        completion.m_group = read->m_group;
        completion.m_bytesRead = bytesTransferred;

        CloseRead(read);
        --m_outstandingReadCount;
        DispatchQueuedReads();
        return true;
    }


    void OverlappedAsyncIOBackend::Cancel(const AsyncReadHandle handle)
    {
        PendingRead* read = FindRead(handle);
        if (read && read->m_file != INVALID_HANDLE_VALUE)
            CancelIoEx(read->m_file, &read->m_overlapped);

        for (uint32_t index = 0; index < m_queuedReads.size(); ++index)
        {
            if (m_queuedReads[index]->m_handle == handle)
            {
                PendingRead* queuedRead = m_queuedReads[index];
                m_queuedReads.erase(m_queuedReads.begin() + index);
                PostQueuedCompletionStatus(m_completionPort,
                                           0,
                                           static_cast<ULONG_PTR>(queuedRead->m_handle.m_value),
                                           &queuedRead->m_overlapped);
                m_reads.push_back(queuedRead);
                ++m_outstandingReadCount;
                return;
            }
        }
    }


    void OverlappedAsyncIOBackend::Tick()
    {
        DispatchQueuedReads();
    }


    void OverlappedAsyncIOBackend::CloseRead(PendingRead* read)
    {
        Memory::DefaultDelete(read);
    }


    OverlappedAsyncIOBackend::PendingRead* OverlappedAsyncIOBackend::FindRead(const AsyncReadHandle handle)
    {
        for (PendingRead* read : m_reads)
        {
            if (read->m_handle == handle)
                return read;
        }

        return nullptr;
    }


    void OverlappedAsyncIOBackend::DispatchQueuedReads()
    {
        while (m_outstandingReadCount < kMaxOutstandingReads && !m_queuedReads.empty())
        {
            PendingRead* read = m_queuedReads.front();
            m_queuedReads.erase(m_queuedReads.begin());
            StartRead(read);
        }
    }
} // namespace FE::IO
