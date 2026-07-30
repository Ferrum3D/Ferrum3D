#include <Core/IO/AsyncImpl.h>
#include <Core/IO/Platform/Windows/OverlappedAsyncIOBackend.h>
#include <Core/Platform/Windows/Common.h>
#include <Core/Strings/Encoding.h>

namespace FE::IO::Async
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


    OverlappedAsyncIOBackend::OverlappedAsyncIOBackend(Threading::Event& completionEvent)
        : m_completionEvent(completionEvent)
    {
        m_pendingRequests.reserve(32);

        m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        m_iocpThreadQuitEvent = Threading::Event::CreateAutoReset();
        m_iocpWaitThread.Start("IOCP Wait Thread", [this] {
            for (;;)
            {
                DWORD bytes = 0;
                ULONG_PTR key = 0;
                OVERLAPPED* overlapped = nullptr;
                const bool success = GetQueuedCompletionStatus(m_completionPort, &bytes, &key, &overlapped, INFINITE);
                if (overlapped)
                {
                    auto* read = CONTAINING_RECORD(overlapped, NativeReadRequest, m_overlapped);
                    if (success)
                    {
                        read->m_completion.m_result = ResultCode::kSuccess;
                        read->m_completion.m_bytesRead = bytes;
                    }
                    else
                    {
                        const DWORD error = GetLastError();
                        read->m_completion.m_result = ConvertWin32OverlappedIOError(error);
                    }

                    m_finishedReadsQueue.Enqueue(read);
                    m_completionEvent.Send();
                }
                else
                {
                    const auto command = static_cast<IocpThreadCommand>(key);
                    FE_Assert(command == IocpThreadCommand::kExit);
                    m_iocpThreadQuitEvent.Send();
                    break;
                }
            }
        });
    }


    OverlappedAsyncIOBackend::~OverlappedAsyncIOBackend()
    {
        PostQueuedCompletionStatus(m_completionPort, 0, festd::to_underlying(IocpThreadCommand::kExit), nullptr);
        m_iocpThreadQuitEvent.Wait();

        FE_Assert(m_completionPort != nullptr);
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
        const Platform::FileHandle fileHandle = read.m_file->GetFileHandle();

        uint32_t requestIndex = m_freeRequests.find_first();
        if (requestIndex == kInvalidIndex)
        {
            constexpr uint32_t growth = decltype(m_readRequests)::kElementsPerSegment;
            m_readRequests.resize(m_readRequests.size() + growth);
            m_freeRequests.resize(m_freeRequests.size() + growth, true);
            FE_AssertDebug(m_readRequests.size() == m_freeRequests.size());

            requestIndex = m_freeRequests.find_first();
        }

        m_freeRequests.reset(requestIndex);

        auto& readRequest = m_readRequests[requestIndex];
        readRequest.m_file = reinterpret_cast<HANDLE>(fileHandle.m_value);
        readRequest.m_destination = read.m_destination;
        readRequest.m_size = read.m_size;
        readRequest.m_overlapped.Offset = static_cast<DWORD>(read.m_offset & Constants::kMaxU32);
        readRequest.m_overlapped.OffsetHigh = static_cast<DWORD>((read.m_offset >> 32) & Constants::kMaxU32);
        readRequest.m_completion.m_group = read.m_group;
        readRequest.m_completion.m_handle = AsyncReadHandle{ requestIndex };

        if (m_dispatchedReadCount < kMaxOutstandingReads)
        {
            StartRead(readRequest);
        }
        else
        {
            if (m_pendingRequests.full())
                m_pendingRequests.resize(m_pendingRequests.size() * 2);

            m_pendingRequests.push_back(requestIndex);
        }

        return readRequest.m_completion.m_handle;
    }


    void OverlappedAsyncIOBackend::StartRead(NativeReadRequest& read)
    {
        FE_Assert(read.m_file != INVALID_HANDLE_VALUE);
        FE_Assert(read.m_size <= Constants::kMaxValue<DWORD>);

        CreateIoCompletionPort(read.m_file, m_completionPort, reinterpret_cast<ULONG_PTR>(&read), 0);

        DWORD bytesRead = 0;
        if (ReadFile(read.m_file, read.m_destination, static_cast<DWORD>(read.m_size), &bytesRead, &read.m_overlapped))
        {
            m_finishedReads.push_back(&read);
        }
        else
        {
            const DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING)
            {
                read.m_completion.m_result = ConvertWin32OverlappedIOError(error);
                m_finishedReads.push_back(&read);
            }
        }
    }


    bool OverlappedAsyncIOBackend::PollRequestCompletion(AsyncIOCompletion& completion)
    {
        if (m_finishedReads.empty())
        {
            auto* requestList = static_cast<NativeReadRequest*>(m_finishedReadsQueue.DequeueAll());
            if (requestList == nullptr)
                return false;

            while (requestList)
            {
                m_finishedReads.push_back(requestList);
                FE_AssertDebug(requestList != static_cast<NativeReadRequest*>(requestList->m_next));
                requestList = static_cast<NativeReadRequest*>(requestList->m_next);
            }
        }

        NativeReadRequest* finishedRequest = m_finishedReads.back();
        m_finishedReads.pop_back();

        completion = finishedRequest->m_completion;
        m_freeRequests.set(completion.m_handle.m_value);
        m_readRequests[completion.m_handle.m_value].Invalidate();
        DispatchQueuedReads();
        return true;
    }


    void OverlappedAsyncIOBackend::Cancel(const AsyncReadHandle handle)
    {
        NativeReadRequest& read = m_readRequests[handle.m_value];
        if (read.m_file != INVALID_HANDLE_VALUE)
            CancelIoEx(read.m_file, &read.m_overlapped);
    }


    void OverlappedAsyncIOBackend::DispatchQueuedReads()
    {
        while (m_dispatchedReadCount < kMaxOutstandingReads && !m_pendingRequests.empty())
        {
            const uint32_t pendingRequestIndex = m_pendingRequests.front();
            m_pendingRequests.pop_front();

            if (pendingRequestIndex == kInvalidIndex)
                break;

            StartRead(m_readRequests[pendingRequestIndex]);
        }
    }
} // namespace FE::IO::Async
