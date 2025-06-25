#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/IO/AsyncStreamIO.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Memory/SegmentedBuffer.h>

namespace FE::IO
{
    namespace
    {
        constexpr uint32_t kSuccessColor = 0x4b4e6d;
        constexpr uint32_t kFailureColor = 0x9a031e;


        struct PageDecompressJob : public Job
        {
            void Execute() override
            {
                FE_PROFILER_ZONE();

                const auto decompressor = Compression::Decompressor::Create(m_method);

                const auto decompressionResult =
                    decompressor.Decompress(m_page + 1, m_page->m_compressedSize, m_destinationBuffer, m_decompressedSize);

                if (decompressionResult.m_result != Compression::ResultCode::kSuccess)
                {
                    m_success = false;
                    return;
                }

                if (decompressionResult.m_decompressedSize != m_decompressedSize)
                {
                    m_success = false;
                    return;
                }

                m_success = true;
            }

            Compression::PageHeader* m_page = nullptr;
            std::byte* m_destinationBuffer = nullptr;
            size_t m_decompressedSize = 0;
            Compression::Method m_method = Compression::Method::kInvalid;
            bool m_success = false;
        };


        struct BlockDecompressJob : public Job
        {
            void Execute() override
            {
                FE_PROFILER_ZONE();

                Memory::FiberTempAllocator temp;

                auto& request = m_entry->m_request;

                const uint32_t blockOffset = m_blockIndex * Compression::kBlockSize;
                std::byte* readPtr = request.m_readBuffer + blockOffset;

                const Rc completionWaitGroup = WaitGroup::Create(static_cast<int32_t>(m_pageBuffer.m_segmentCount));
                SegmentedVector<PageDecompressJob> childJobs{ &temp };

                for (uint32_t pageIndex = 0; pageIndex < m_pageBuffer.m_segmentCount; ++pageIndex)
                {
                    auto* page = reinterpret_cast<Compression::PageHeader*>(m_pageBuffer.m_segments[pageIndex] + 1);

                    size_t decompressedSize = m_pageDecompressedSize;
                    if (pageIndex == m_pageBuffer.m_segmentCount - 1)
                        decompressedSize = m_tailPageDecompressedSize;

                    PageDecompressJob& job = childJobs.push_back();
                    job.m_page = page;
                    job.m_destinationBuffer = readPtr;
                    job.m_decompressedSize = decompressedSize;
                    job.m_method = m_method;
                    job.ScheduleBackground(m_jobSystem, completionWaitGroup.Get(), request.m_decompressionPriority);

                    readPtr += decompressedSize;
                }

                completionWaitGroup->Wait();

                m_entry->m_status.store(AsyncOperationStatus::kSucceeded, std::memory_order_release);

                bool success = true;
                for (const PageDecompressJob& job : childJobs)
                {
                    if (!job.m_success)
                    {
                        success = false;
                        break;
                    }
                }

                m_entry->m_status.store(success ? AsyncOperationStatus::kSucceeded : AsyncOperationStatus::kFailed,
                                        std::memory_order_release);

                m_pageBuffer.Free();

                AsyncBlockReadResult result;
                result.m_controller = m_entry->m_controller.Get();
                result.m_request = &request;
                result.m_bytesRead = readPtr - request.m_readBuffer - blockOffset;
                result.m_blockIndex = m_blockIndex;
                request.m_callback->AsyncIOCallback(result);

                if (m_entry->m_remainingBlockCount.fetch_sub(1, std::memory_order_release) == 1)
                    Memory::Delete(m_entryAllocator, m_entry);

                Memory::Delete(m_jobAllocator, this);
            }

            IJobSystem* m_jobSystem;
            AsyncBlockReadRequestQueueEntry* m_entry;
            Memory::SpinLockedPoolAllocator* m_entryAllocator;
            Memory::SpinLockedPoolAllocator* m_jobAllocator;
            uint32_t m_pageDecompressedSize;
            uint32_t m_tailPageDecompressedSize;
            Compression::Method m_method;
            uint32_t m_blockIndex;
            Memory::SegmentedBuffer m_pageBuffer;
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


    void AsyncStreamIO::ProcessRequest(AsyncReadRequestQueueEntry* entry, AsyncOperationStatus status)
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
            Memory::Delete(&m_requestPools[festd::to_underlying(AsyncReadRequestQueueEntry::Type::kRead)], entry);
        });

        if (status == AsyncOperationStatus::kFailed || status == AsyncOperationStatus::kCanceled)
            return;

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

        if (request.m_readBufferSize == 0)
            request.m_readBufferSize = static_cast<uint32_t>(request.m_stream->Length() - request.m_offset);

        if (request.m_readBuffer == nullptr)
        {
            const uint32_t allocBytes = request.m_readBufferSize + request.m_overallocateBytes;
            request.m_readBuffer = static_cast<std::byte*>(request.m_allocator->allocate(allocBytes, Memory::kDefaultAlignment));
        }

        result.m_bytesRead = request.m_stream->ReadToBuffer(request.m_readBuffer, request.m_readBufferSize);
        request.m_offset += result.m_bytesRead;
        status = AsyncOperationStatus::kSucceeded;
        ZoneColor(kSuccessColor);
    }


    void AsyncStreamIO::ProcessRequest(AsyncBlockReadRequestQueueEntry* entry, AsyncOperationStatus status)
    {
        FE_PROFILER_ZONE_NAMED("AsyncBlockReadRequest");

        AsyncBlockReadRequest& request = entry->m_request;

        FE_Assert(request.m_blockCount > 0);

        if (request.m_allocator == nullptr)
            request.m_allocator = std::pmr::get_default_resource();

        AsyncBlockReadResult result{};
        result.m_controller = entry->m_controller.Get();
        result.m_request = &request;

        auto deferCallback = festd::defer([&] {
            entry->m_status.store(status, std::memory_order_release);
            request.m_callback->AsyncIOCallback(result);
            Memory::Delete(&m_requestPools[festd::to_underlying(AsyncBlockReadRequestQueueEntry::Type::kReadBlock)], entry);
        });

        if (status == AsyncOperationStatus::kFailed || status == AsyncOperationStatus::kCanceled)
            return;

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

        festd::inline_vector<BlockDecompressJob*, 64> jobs;

        auto deleteJobsIfFailed = festd::defer([&] {
            for (auto* job : jobs)
                Memory::Delete(&m_blockDecompressionJobPool, job);
        });

        for (uint32_t blockIndex = 0; blockIndex < request.m_blockCount; ++blockIndex)
        {
            FE_PROFILER_ZONE_NAMED("ProcessBlock");

            Compression::BlockHeader blockHeader;
            if (!request.m_stream->Read(blockHeader))
            {
                status = AsyncOperationStatus::kFailed;
                entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                ZoneColor(kFailureColor);
                return;
            }

            request.m_offset += sizeof(Compression::BlockHeader);

            if (request.m_decompress)
            {
                const Compression::Method method = Compression::DecodeMagic(blockHeader.m_magic);
                if (method == Compression::Method::kInvalid)
                {
                    status = AsyncOperationStatus::kFailed;
                    entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                    ZoneColor(kFailureColor);
                    return;
                }

                if (request.m_readBuffer == nullptr)
                {
                    const uint32_t allocBytes = Compression::kBlockSize * request.m_blockCount;
                    request.m_readBuffer =
                        static_cast<std::byte*>(request.m_allocator->allocate(allocBytes, Memory::kDefaultAlignment));
                }

                Memory::SegmentedBufferManualBuilder pageBufferBuilder{ std::pmr::get_default_resource() };
                for (;;)
                {
                    Compression::PageHeader pageHeader;
                    if (!request.m_stream->Read(pageHeader))
                    {
                        status = AsyncOperationStatus::kFailed;
                        entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                        ZoneColor(kFailureColor);
                        return;
                    }

                    request.m_offset += sizeof(Compression::PageHeader);

                    const uint32_t segmentSize = pageHeader.m_compressedSize + sizeof(Compression::PageHeader);
                    std::byte* segment = pageBufferBuilder.AllocateSegment(segmentSize);
                    memcpy(segment, &pageHeader, sizeof(Compression::PageHeader));
                    segment += sizeof(Compression::PageHeader);
                    if (request.m_stream->ReadToBuffer(segment, pageHeader.m_compressedSize) != pageHeader.m_compressedSize)
                    {
                        status = AsyncOperationStatus::kFailed;
                        entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                        ZoneColor(kFailureColor);
                        return;
                    }

                    request.m_offset += pageHeader.m_compressedSize;

                    if (pageHeader.m_nextPageOffset == kInvalidIndex)
                        break;

                    if (pageHeader.m_nextPageOffset != pageHeader.m_compressedSize)
                    {
                        if (pageHeader.m_nextPageOffset < pageHeader.m_compressedSize)
                        {
                            status = AsyncOperationStatus::kFailed;
                            entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                            ZoneColor(kFailureColor);
                            return;
                        }

                        request.m_stream->Seek(pageHeader.m_nextPageOffset - pageHeader.m_compressedSize, SeekMode::kCurrent);
                    }
                }

                Compression::BlockFooter blockFooter;
                if (!request.m_stream->Read(blockFooter))
                {
                    status = AsyncOperationStatus::kFailed;
                    entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                    ZoneColor(kFailureColor);
                    return;
                }

                request.m_offset += sizeof(Compression::BlockFooter);

                auto* decompressionJob = Memory::New<BlockDecompressJob>(&m_blockDecompressionJobPool);
                decompressionJob->m_jobSystem = m_jobSystem;
                decompressionJob->m_entry = entry;
                decompressionJob->m_entryAllocator =
                    &m_requestPools[festd::to_underlying(AsyncBlockReadRequestQueueEntry::Type::kReadBlock)];
                decompressionJob->m_jobAllocator = &m_blockDecompressionJobPool;
                decompressionJob->m_pageDecompressedSize = blockHeader.m_uncompressedPageSize;
                decompressionJob->m_tailPageDecompressedSize = blockFooter.m_tailPageUncompressedSize;
                decompressionJob->m_method = method;
                decompressionJob->m_blockIndex = blockIndex;
                decompressionJob->m_pageBuffer = pageBufferBuilder.Build();

                jobs.push_back(decompressionJob);
            }
            else
            {
                FE_Assert(request.m_readBuffer != nullptr || request.m_readBufferSize == 0,
                          "When not decompressing, the read buffer cannot be allocated automatically");

                Memory::BlockWriter writer{ request.m_readBuffer, request.m_readBufferSize };
                writer.Write(blockHeader);

                for (;;)
                {
                    Compression::PageHeader pageHeader;
                    if (!request.m_stream->Read(pageHeader))
                    {
                        status = AsyncOperationStatus::kFailed;
                        entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                        ZoneColor(kFailureColor);
                        return;
                    }

                    request.m_offset += sizeof(Compression::PageHeader);

                    writer.Write(pageHeader);

                    if (request.m_stream->ReadToBuffer(writer.AllocateSpan(pageHeader.m_compressedSize))
                        != pageHeader.m_compressedSize)
                    {
                        status = AsyncOperationStatus::kFailed;
                        entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                        ZoneColor(kFailureColor);
                        return;
                    }

                    request.m_offset += pageHeader.m_compressedSize;

                    if (pageHeader.m_nextPageOffset == kInvalidIndex)
                        break;

                    if (pageHeader.m_nextPageOffset != pageHeader.m_compressedSize)
                    {
                        if (pageHeader.m_nextPageOffset < pageHeader.m_compressedSize)
                        {
                            status = AsyncOperationStatus::kFailed;
                            entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                            ZoneColor(kFailureColor);
                            return;
                        }

                        request.m_stream->Seek(pageHeader.m_nextPageOffset - pageHeader.m_compressedSize, SeekMode::kCurrent);
                    }
                }

                Compression::BlockFooter blockFooter;
                if (!request.m_stream->Read(blockFooter))
                {
                    status = AsyncOperationStatus::kFailed;
                    entry->m_lastResult.store(ResultCode::kInvalidFormat, std::memory_order_release);
                    ZoneColor(kFailureColor);
                    return;
                }

                request.m_offset += sizeof(Compression::BlockFooter);

                writer.Write(blockFooter);

                result.m_bytesRead = writer.m_ptr - request.m_readBuffer;
                result.m_blockIndex = blockIndex;
                request.m_callback->AsyncIOCallback(result);
            }
        }

        // If we have reached this point, the request processing has been successful so far.
        // Which means we can schedule the jobs, and they will call the callbacks when done.
        deferCallback.dismiss();

        // They will also delete themselves.
        deleteJobsIfFailed.dismiss();

        entry->m_remainingBlockCount = jobs.size();

        for (auto* job : jobs)
        {
            job->ScheduleBackground(m_jobSystem, nullptr, request.m_decompressionPriority);
        }
    }


    void AsyncStreamIO::ProcessGenericRequest(AsyncRequestQueueEntry* entry)
    {
        FE_PROFILER_ZONE_NAMED("ProcessRequest");

        auto status = AsyncOperationStatus::kSucceeded;
        if (entry->m_status.load(std::memory_order_relaxed) == AsyncOperationStatus::kCanceled)
            status = AsyncOperationStatus::kCanceled;

        AsyncOperationRequest& request = *entry->m_requestPtr;
        if (request.m_stream == nullptr && status != AsyncOperationStatus::kCanceled)
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
            }
        }

        if (request.m_path.empty())
            request.m_path = request.m_stream->GetName();

        switch (entry->m_type)
        {
        default:
        case AsyncRequestQueueEntry::Type::kCount:
            FE_DebugBreak();
            [[fallthrough]];

        case AsyncRequestQueueEntry::Type::kRead:
            ProcessRequest(static_cast<AsyncReadRequestQueueEntry*>(entry), status);
            break;
        case AsyncRequestQueueEntry::Type::kReadBlock:
            ProcessRequest(static_cast<AsyncBlockReadRequestQueueEntry*>(entry), status);
            break;
        }
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

            if (entry->m_cancellationRequested.load(std::memory_order_acquire))
            {
                entry->m_status.store(AsyncOperationStatus::kCanceled, std::memory_order_release);
            }
            else
            {
                entry->m_status.store(AsyncOperationStatus::kRunning, std::memory_order_release);
            }

            ProcessGenericRequest(entry);
        }
    }


    AsyncStreamIO::AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IStreamFactory* streamFactory)
        : m_logger(logger)
        , m_jobSystem(jobSystem)
        , m_streamFactory(streamFactory)
    {
        m_requestPools[festd::to_underlying(AsyncReadRequestQueueEntry::Type::kRead)].Initialize(
            "AsyncReadRequestPool", sizeof(AsyncReadRequestQueueEntry));
        m_requestPools[festd::to_underlying(AsyncBlockReadRequestQueueEntry::Type::kReadBlock)].Initialize(
            "AsyncBlockReadRequestPool", sizeof(AsyncBlockReadRequestQueueEntry));

        m_blockDecompressionJobPool.Initialize("AsyncBlockDecompressionJobPool", sizeof(BlockDecompressJob));

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

        constexpr uint32_t poolIndex = festd::to_underlying(AsyncReadRequestQueueEntry::Type::kRead);
        auto* entry = Memory::New<AsyncReadRequestQueueEntry>(&m_requestPools[poolIndex]);
        auto* controller = Rc<AsyncController>::New(&m_controllerPool, entry);
        entry->m_type = AsyncRequestQueueEntry::Type::kRead;
        entry->m_priority = priority;
        entry->m_request = request;
        entry->m_requestPtr = &entry->m_request;
        entry->m_controller = controller;

        EnqueueImpl(priority, entry);

        if (ppController)
            *ppController = controller;

        m_queueEvent.Send();
    }


    void AsyncStreamIO::ReadAsync(const AsyncBlockReadRequest& request, const Priority priority, IAsyncController** ppController)
    {
        std::lock_guard lk{ m_queueLock };

        FE_Assert(request.m_blockCount > 0);

        constexpr uint32_t poolIndex = festd::to_underlying(AsyncBlockReadRequestQueueEntry::Type::kReadBlock);
        auto* entry = Memory::New<AsyncBlockReadRequestQueueEntry>(&m_requestPools[poolIndex]);
        auto* controller = Rc<AsyncController>::New(&m_controllerPool, entry);
        entry->m_type = AsyncRequestQueueEntry::Type::kReadBlock;
        entry->m_priority = priority;
        entry->m_request = request;
        entry->m_requestPtr = &entry->m_request;
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
