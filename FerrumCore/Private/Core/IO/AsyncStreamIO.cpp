#include <Core/IO/AsyncStreamIO.h>
#include <Core/Jobs/Job.h>
#include <Core/Logging/Trace.h>

namespace FE::IO
{
    namespace
    {
        constexpr uint32_t kSuccessColor = 0x4b4e6d;
        constexpr uint32_t kFailureColor = 0x9a031e;


        struct DecompressionJob final : public Job
        {
            void Execute() override
            {
                FE_PROFILER_ZONE();

                AsyncReadRequest& request = m_entry->m_request;
                AsyncReadResult result{};
                result.m_controller = m_entry->m_controller.Get();
                result.m_request = &request;

                AsyncOperationStatus status;
                if (m_entry->m_cancellationRequested.load(std::memory_order_acquire))
                {
                    status = AsyncOperationStatus::kCanceled;
                    m_entry->m_lastResult.store(ResultCode::kCanceled, std::memory_order_release);
                }
                else
                {
                    const auto decompressor = Compression::Decompressor::Create(request.m_compressionMethod);
                    const Compression::DecompressionResult decompressionResult =
                        decompressor.Decompress(m_compressedBuffer,
                                                request.m_compressedSize,
                                                request.m_readBuffer,
                                                request.m_readBufferSize);

                    if (decompressionResult.m_result == Compression::ResultCode::kSuccess)
                    {
                        status = AsyncOperationStatus::kSucceeded;
                        result.m_bytesRead = decompressionResult.m_decompressedSize;
                    }
                    else
                    {
                        status = AsyncOperationStatus::kFailed;
                        m_entry->m_lastResult.store(ResultCode::kDecompressionError, std::memory_order_release);
                    }
                }

                request.m_allocator->deallocate(m_compressedBuffer, request.m_compressedSize);
                m_entry->m_status.store(status, std::memory_order_release);
                request.m_callback->AsyncIOCallback(result);

                Memory::Delete(m_requestAllocator, m_entry);
                Memory::Delete(m_jobAllocator, this);
            }

            AsyncRequestQueueEntry* m_entry = nullptr;
            Memory::SpinLockedPoolAllocator* m_requestAllocator = nullptr;
            Memory::SpinLockedPoolAllocator* m_jobAllocator = nullptr;
            std::byte* m_compressedBuffer = nullptr;
        };
    } // namespace


    AsyncRequestQueueEntry* AsyncStreamIO::TryDequeue()
    {
        FE_PROFILER_ZONE();

        if (m_queue.empty())
            return nullptr;

        AsyncRequestQueueEntry* entry = m_queue.front();
        m_queue.erase(m_queue.begin());
        return entry;
    }


    void AsyncStreamIO::ProcessRequest(AsyncRequestQueueEntry* entry, AsyncOperationStatus status)
    {
        FE_PROFILER_ZONE_NAMED("AsyncReadRequest");

        AsyncReadRequest& request = entry->m_request;

        if (request.m_allocator == nullptr)
            request.m_allocator = std::pmr::get_default_resource();

        AsyncReadResult result{};
        result.m_controller = entry->m_controller.Get();
        result.m_request = &request;

        auto deferCallback = festd::defer([&] {
            entry->m_status.store(status, std::memory_order_release);
            request.m_callback->AsyncIOCallback(result);
            Memory::Delete(&m_requestPool, entry);
        });

        if (status == AsyncOperationStatus::kFailed || status == AsyncOperationStatus::kCanceled)
            return;

        if (request.m_stream == nullptr)
        {
            if (const auto openResult = m_streamFactory->OpenFileStream(request.m_path, OpenMode::kReadOnly))
            {
                request.m_stream = openResult.value();
                const festd::string_view zoneText = request.m_stream->GetName();
                ZoneText(zoneText.data(), zoneText.size());
            }
            else
            {
                const ResultCode errorCode = openResult.error();
                const festd::string_view resultDesc = GetResultDesc(errorCode);
                status = AsyncOperationStatus::kFailed;
                entry->m_lastResult.store(errorCode, std::memory_order_release);
                ZoneTextF("Failed request: %.*s", resultDesc.size(), resultDesc.data());
                ZoneColor(kFailureColor);
                return;
            }
        }

        if (request.m_path.empty())
            request.m_path = request.m_stream->GetName();

        if (request.m_offset > 0)
        {
            const ResultCode seekResult = request.m_stream->Seek(request.m_offset, SeekMode::kBegin);
            if (seekResult != ResultCode::kSuccess)
            {
                status = AsyncOperationStatus::kFailed;
                entry->m_lastResult.store(seekResult, std::memory_order_release);
                ZoneColor(kFailureColor);
                return;
            }
        }

        FE_Assert(request.m_compressionMethod < Compression::Method::kInvalid);

        if (request.m_compressionMethod == Compression::Method::kNone)
        {
            if (request.m_readBufferSize == 0)
                request.m_readBufferSize = static_cast<uint32_t>(request.m_stream->Length() - request.m_offset);

            if (request.m_readBuffer == nullptr)
            {
                const uint32_t allocBytes = request.m_readBufferSize + request.m_overallocateBytes;
                request.m_readBuffer =
                    static_cast<std::byte*>(request.m_allocator->allocate(allocBytes, Memory::kDefaultAlignment));
            }

            result.m_bytesRead = request.m_stream->ReadToBuffer(request.m_readBuffer, request.m_readBufferSize);
            request.m_offset += result.m_bytesRead;
            status = AsyncOperationStatus::kSucceeded;
            ZoneColor(kSuccessColor);
            return;
        }

        if (request.m_compressedSize == 0 || request.m_readBufferSize == 0)
        {
            status = AsyncOperationStatus::kFailed;
            entry->m_lastResult.store(ResultCode::kInvalidArgument, std::memory_order_release);
            ZoneColor(kFailureColor);
            return;
        }

        if (request.m_readBuffer == nullptr)
        {
            const uint32_t allocBytes = request.m_readBufferSize + request.m_overallocateBytes;
            request.m_readBuffer = static_cast<std::byte*>(request.m_allocator->allocate(allocBytes, Memory::kDefaultAlignment));
        }

        auto* compressedBuffer =
            static_cast<std::byte*>(request.m_allocator->allocate(request.m_compressedSize, Memory::kDefaultAlignment));

        const size_t compressedBytesRead = request.m_stream->ReadToBuffer(compressedBuffer, request.m_compressedSize);
        request.m_offset += static_cast<intptr_t>(compressedBytesRead);
        if (compressedBytesRead != request.m_compressedSize)
        {
            request.m_allocator->deallocate(compressedBuffer, request.m_compressedSize);
            status = AsyncOperationStatus::kFailed;
            entry->m_lastResult.store(ResultCode::kIOError, std::memory_order_release);
            ZoneColor(kFailureColor);
            return;
        }

        auto* job = Memory::New<DecompressionJob>(&m_decompressionJobPool);
        job->m_entry = entry;
        job->m_requestAllocator = &m_requestPool;
        job->m_jobAllocator = &m_decompressionJobPool;
        job->m_compressedBuffer = compressedBuffer;

        deferCallback.dismiss();
        job->ScheduleBackground(m_jobSystem, nullptr, request.m_decompressionPriority);
    }


    void AsyncStreamIO::ReaderThread()
    {
        while (true)
        {
            if (m_exitRequested)
                break;

            m_queueEvent.Wait();

            if (m_exitRequested)
                break;

            AsyncRequestQueueEntry* entry;

            {
                std::lock_guard lk{ m_queueLock };
                entry = TryDequeue();
                if (entry == nullptr)
                {
                    m_queueEvent.Reset();
                    continue;
                }
            }

            AsyncOperationStatus status;
            if (entry->m_cancellationRequested.load(std::memory_order_acquire))
                status = AsyncOperationStatus::kCanceled;
            else
                status = AsyncOperationStatus::kRunning;

            entry->m_status.store(status, std::memory_order_release);
            ProcessRequest(entry, status);
        }
    }


    AsyncStreamIO::AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IStreamFactory* streamFactory)
        : m_logger(logger)
        , m_jobSystem(jobSystem)
        , m_streamFactory(streamFactory)
    {
        m_decompressionJobPool.Initialize("IO/Async/DecompressionJobPool", sizeof(DecompressionJob));

        const auto threadFunc = [](const uintptr_t userData) {
            reinterpret_cast<AsyncStreamIO*>(userData)->ReaderThread();
        };

        m_thread = Threading::CreateThread("Async IO Thread", threadFunc, reinterpret_cast<uintptr_t>(this));
        m_queueEvent = Threading::Event::CreateManualReset();
    }


    AsyncStreamIO::~AsyncStreamIO()
    {
        m_exitRequested = true;
        m_queueEvent.Send();
        Threading::CloseThread(m_thread);
    }


    void AsyncStreamIO::ReadAsync(const AsyncReadRequest& request, const Priority priority, IAsyncController** ppController)
    {
        std::lock_guard lk{ m_queueLock };

        auto* entry = Memory::New<AsyncRequestQueueEntry>(&m_requestPool);
        auto* controller = Rc<AsyncController>::New(&m_controllerPool, entry);
        entry->m_priority = priority;
        entry->m_request = request;
        entry->m_controller = controller;

        EnqueueImpl(priority, entry);

        if (ppController)
            *ppController = controller;

        m_queueEvent.Send();
    }


    void AsyncStreamIO::EnqueueImpl(const Priority priority, AsyncRequestQueueEntry* entry)
    {
        const auto iter = festd::upper_bound(m_queue, priority, [](const Priority lhs, const AsyncRequestQueueEntry* rhs) {
            return lhs < rhs->m_priority;
        });

        m_queue.insert(iter, entry);
    }
} // namespace FE::IO
