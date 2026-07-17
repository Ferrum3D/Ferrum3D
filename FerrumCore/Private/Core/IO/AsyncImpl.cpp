#include <Core/IO/AsyncImpl.h>
#include <Core/IO/Platform/PlatformFile.h>
#include <Core/Jobs/JobGraph.h>
#include <Core/Jobs/JobNode.h>
#include <Core/Logging/Trace.h>
#include <Core/Memory/Memory.h>

#if FE_PLATFORM_WINDOWS
#    include <Core/IO/Platform/Windows/OverlappedAsyncIOBackend.h>
#endif
#include <Core/IO/DefaultAsyncIOBackend.h>

namespace FE::IO::Async
{
    namespace
    {
        constexpr size_t kStagingHeapSize = 256 * 1024 * 1024;


        void SetOperationResult(const Operation* operation, const ResultCode result)
        {
            if (result == ResultCode::kSuccess)
                return;

            auto expected = ResultCode::kSuccess;
            operation->m_controller->m_lastResult.compare_exchange_strong(expected, result, std::memory_order_acq_rel);
        }


        void CompleteOperationWork(Operation* operation)
        {
            const uint32_t previousValue = operation->m_pendingWork.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previousValue > 0, "Async operation pending work underflow");
        }

        SchedulerImpl* GScheduler;
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_Assert(GScheduler == nullptr, "AsyncIO already initialized");
        GScheduler = Memory::New<SchedulerImpl>(allocator);
    }


    void Internal::Shutdown()
    {
        FE_Assert(GScheduler != nullptr, "AsyncIO not initialized");
        GScheduler->~SchedulerImpl();
        GScheduler = nullptr;
    }


    SchedulerImpl& SchedulerImpl::Get()
    {
        return *GScheduler;
    }


    Rc<IController> Read(Batch&& batch, const Priority priority)
    {
        return GScheduler->Read(std::move(batch), priority);
    }


    Rc<IController> Read(const Batch& batch, const Priority priority)
    {
        return GScheduler->Read(batch, priority);
    }


    void Batch::SetSource(const ResolvedDataSource& resolvedDataSource)
    {
        FE_Assert(!m_resolvedDataSource.IsValid());
        m_resolvedDataSource = resolvedDataSource;
    }


    void Batch::SetSource(const festd::string_view filePath, const size_t fileSize)
    {
        FE_Assert(!m_resolvedDataSource.IsValid());
        m_resolvedDataSource = ResolvedDataSource{
            .m_filePath = filePath,
            .m_byteOffset = 0,
            .m_byteSize = fileSize,
        };
    }


    void Batch::SetSource(const PathView& filePath, const size_t fileSize)
    {
        FE_Assert(!m_resolvedDataSource.IsValid());
        m_resolvedDataSource = ResolvedDataSource{
            .m_filePath = filePath,
            .m_byteOffset = 0,
            .m_byteSize = fileSize,
        };
    }


    void Batch::SetCompletionWaitGroup(WaitGroup* waitGroup)
    {
        FE_Assert(waitGroup);
        FE_Assert(!m_completionWaitGroup);
        m_completionWaitGroup = waitGroup;
    }


    void Batch::Read(void* destination, const size_t destinationSize, const size_t sourceOffset)
    {
        ValidateRead(sourceOffset, destinationSize);

        Command command;
        command.m_destination.m_byteBuffer = static_cast<std::byte*>(destination);
        command.m_sourceOffset = sourceOffset;
        command.m_compressedSize = destinationSize;
        command.m_uncompressedSize = destinationSize;
        command.m_compressionMethod = Compression::Method::kNone;
        command.m_vectorDestination = false;
        m_commands.push_back(command);
    }


    void Batch::Read(void* destination, const size_t destinationSize, const size_t compressedSize,
                     const Compression::Method compressionMethod, const size_t sourceOffset)
    {
        ValidateRead(sourceOffset, compressedSize);

        Command command;
        command.m_destination.m_byteBuffer = static_cast<std::byte*>(destination);
        command.m_sourceOffset = sourceOffset;
        command.m_compressedSize = compressedSize;
        command.m_uncompressedSize = destinationSize;
        command.m_compressionMethod = compressionMethod;
        command.m_vectorDestination = false;
        m_commands.push_back(command);
    }


    void Batch::Read(const festd::span<std::byte> destination, const size_t sourceOffset)
    {
        Read(destination.data(), destination.size_bytes(), sourceOffset);
    }


    void Batch::Read(const festd::span<std::byte> destination, const size_t compressedSize,
                     const Compression::Method compressionMethod, const size_t sourceOffset)
    {
        Read(destination.data(), destination.size_bytes(), compressedSize, compressionMethod, sourceOffset);
    }


    void Batch::ReadAppend(festd::pmr::vector<std::byte>& destination, const size_t bytesToRead, const size_t sourceOffset)
    {
        Command command;
        command.m_destination.m_vector = &destination;
        command.m_sourceOffset = sourceOffset;
        command.m_compressedSize = bytesToRead;
        command.m_uncompressedSize = bytesToRead;
        command.m_compressionMethod = Compression::Method::kNone;
        command.m_vectorDestination = true;
        m_commands.push_back(command);
    }


    void Batch::ReadAppend(festd::pmr::vector<std::byte>& destination, const Compression::Method compressionMethod,
                           const size_t compressedSize, const size_t uncompressedSize, const size_t sourceOffset)
    {
        ValidateRead(sourceOffset, compressedSize);

        Command command;
        command.m_destination.m_vector = &destination;
        command.m_sourceOffset = sourceOffset;
        command.m_compressedSize = compressedSize;
        command.m_uncompressedSize = uncompressedSize;
        command.m_compressionMethod = compressionMethod;
        command.m_vectorDestination = true;
        m_commands.push_back(command);
    }


    void Batch::ValidateRead(const size_t offset, const size_t byteSize) const
    {
        FE_Assert(m_resolvedDataSource.IsValid());

        if (m_resolvedDataSource.m_byteSize != 0)
            FE_Assert(offset + byteSize <= m_resolvedDataSource.m_byteSize);
    }


    void Controller::DoRelease()
    {
        m_pool.Delete(this);
    }


    void Controller::Cancel()
    {
        m_cancellationRequested.store(true, std::memory_order_release);
    }


    Status Controller::GetStatus() const
    {
        return m_status.load(std::memory_order_acquire);
    }


    ResultCode Controller::GetLastOperationResult() const
    {
        return m_lastResult.load(std::memory_order_acquire);
    }


    void CachedFile::DoRelease()
    {
        m_pool.Delete(this);
    }


    void OpenFileCache::Init(const uint32_t cacheSize, IAsyncIOBackend* backend)
    {
        m_backend = backend;
        m_cacheSize = cacheSize;
        m_entries.reserve(cacheSize);
    }


    void OpenFileCache::Shutdown()
    {
        for (const Rc<CachedFile>& entry : m_entries)
            Platform::CloseFile(entry->m_fileHandle);

        m_cacheSize = 0;
        m_entries.clear();
    }


    festd::expected<Rc<CachedFile>, ResultCode> OpenFileCache::CreateFile(const festd::string_view path)
    {
        FE_AssertDebug(m_cacheSize > 0 && m_backend != nullptr);

        const uint64_t hash = DefaultHash(path);
        for (uint32_t entryIndex = 0; entryIndex < m_entries.size(); ++entryIndex)
        {
            auto entry = m_entries[entryIndex];
            if (entry->m_nameHash == hash && entry->m_path == path)
            {
                m_entries.erase(m_entries.begin() + entryIndex);
                m_entries.push_back(entry);
                return entry;
            }
        }

        const festd::expected openFileResult = m_backend->OpenFile(path);
        if (!openFileResult.has_value())
            return festd::unexpected(openFileResult.error());

        if (m_entries.size() >= m_cacheSize)
        {
            for (uint32_t entryIndex = 0; entryIndex < m_entries.size(); ++entryIndex)
            {
                if (m_entries[entryIndex]->GetRefCount() == 1) // cache is the only owner
                {
                    DeleteEntry(entryIndex);
                    --entryIndex;

                    if (m_entries.size() < m_cacheSize)
                        break;
                }
            }
        }

        Rc newEntry = m_entryPool.New();
        newEntry->m_nameHash = hash;
        newEntry->m_path = path;
        newEntry->m_fileHandle = openFileResult.value();
        m_entries.push_back(newEntry);
        return newEntry;
    }


    void OpenFileCache::CollectGarbage()
    {
        const uint64_t timestamp = Platform::GetTicks();
        const double ticksPerSecond = Platform::GetSecondsPerTick();
        for (uint32_t entryIndex = 0; entryIndex < m_entries.size(); ++entryIndex)
        {
            if (m_entries[entryIndex]->GetRefCount() == 1) // cache is the only owner
            {
                const Rc<CachedFile> entry = m_entries[entryIndex];
                const double elapsedSeconds = static_cast<double>(timestamp - entry->m_lastUseTime) * ticksPerSecond;
                if (elapsedSeconds > 20)
                {
                    DeleteEntry(entryIndex);
                    --entryIndex;
                }
            }
        }
    }


    void OpenFileCache::DeleteEntry(const uint32_t entryIndex)
    {
        Rc<CachedFile> entry = m_entries[entryIndex];
        m_entries.erase(m_entries.begin() + entryIndex);
        Platform::CloseFile(entry->m_fileHandle);
        entry->m_fileHandle.Reset();
    }


    void SchedulerImpl::EnqueueImpl(Operation* operation)
    {
        std::unique_lock lk{ m_queueLock };

        const auto iter = festd::upper_bound(m_queue, operation->m_priority, [](const Priority lhs, const Operation* rhs) {
            return lhs < rhs->m_priority;
        });

        m_queue.insert(iter, operation);
    }


    Operation* SchedulerImpl::TryDequeue()
    {
        std::unique_lock lk{ m_queueLock };
        if (m_queue.empty())
            return nullptr;

        Operation* operation = m_queue.front();
        m_queue.erase(m_queue.begin());
        return operation;
    }


    void SchedulerImpl::ProcessOperation(Operation* operation)
    {
        FE_PROFILER_ZONE();

        operation->m_controller->m_status.store(Status::kRunning, std::memory_order_release);

        const bool canceledOnStart = operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire);
        if (canceledOnStart)
        {
            SetOperationResult(operation, ResultCode::kCanceled);
            return;
        }

        const ResolvedDataSource dataSource = operation->m_batch.m_resolvedDataSource;
        for (const Batch::Command& command : operation->m_batch.m_commands)
        {
            const festd::expected fileOpenResult = m_fileCache.CreateFile(dataSource.m_filePath);
            if (!fileOpenResult.has_value())
            {
                SetOperationResult(operation, fileOpenResult.error());
                break;
            }

            Rc file = fileOpenResult.value();

            std::byte* destination;
            if (command.m_vectorDestination)
            {
                FileStats fileStats;
                FE_Verify(Platform::GetFileStats(file->GetFileHandle(), fileStats) == ResultCode::kSuccess);

                auto& v = *command.m_destination.m_vector;
                const uint32_t initialSize = v.size();
                v.resize(static_cast<uint32_t>(initialSize + fileStats.m_byteSize - command.m_sourceOffset));
                destination = v.data() + initialSize;
            }
            else
            {
                destination = command.m_destination.m_byteBuffer;
            }

            auto* group = m_groupPool.New();
            group->m_sourcePath = dataSource.m_filePath;
            group->m_operation = operation;
            group->m_destination = destination;
            group->m_destinationSize = command.m_uncompressedSize;
            group->m_readSize = command.m_compressedSize;
            group->m_compressionMethod = command.m_compressionMethod;

            void* readDestination = destination;
            if (command.m_compressionMethod != Compression::Method::kNone)
            {
                group->m_stagingMemorySize = command.m_compressedSize;
                group->m_stagingMemory = m_stagingAllocator.allocate(command.m_compressedSize, Memory::kDefaultAlignment);

                readDestination = group->m_stagingMemory;
            }

            operation->m_pendingWork.fetch_add(1, std::memory_order_acq_rel);

            AsyncIOPhysicalRead read;
            read.m_file = std::move(file);
            read.m_offset = dataSource.m_byteOffset + command.m_sourceOffset;
            read.m_destination = readDestination;
            read.m_size = command.m_compressedSize;
            read.m_group = group;
            group->m_handle = m_backend->DispatchRead(read);
        }
    }


    void SchedulerImpl::ProcessBackendCompletions()
    {
        Jobs::Graph tg{ "IO/Async/RequestGraph", Jobs::FiberAffinityMask::kAllBackground };

        AsyncIOCompletion completion;
        while (m_backend->PollRequestCompletion(completion))
        {
            ReadGroup* group = completion.m_group;
            if (group == nullptr)
                continue;

            Operation* operation = group->m_operation;
            ResultCode result = completion.m_result;
            if (result == ResultCode::kSuccess && completion.m_bytesRead != group->m_readSize)
            {
                if (group->m_compressionMethod != Compression::Method::kNone)
                    result = ResultCode::kDecompressionError;
            }

            if (operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire))
                result = ResultCode::kCanceled;

            if (result != ResultCode::kSuccess)
            {
                SetOperationResult(operation, result);
                if (group->m_stagingMemory != nullptr)
                    m_stagingAllocator.deallocate(group->m_stagingMemory, group->m_stagingMemorySize);

                CompleteOperationWork(operation);
                m_groupPool.Delete(group);
                continue;
            }

            if (group->m_compressionMethod == Compression::Method::kNone)
            {
                CompleteOperationWork(operation);
                m_groupPool.Delete(group);
            }
            else
            {
                tg.Dispatch("Block", [this, operation, group] {
                    auto blockResult = ResultCode::kSuccess;
                    if (operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire))
                    {
                        blockResult = ResultCode::kCanceled;
                    }
                    else
                    {
                        const auto decompressor = Compression::Decompressor::Create(group->m_compressionMethod);
                        const Compression::DecompressionResult decompressionResult =
                            decompressor.Decompress(group->m_stagingMemory,
                                                    group->m_stagingMemorySize,
                                                    group->m_destination,
                                                    group->m_destinationSize);

                        if (decompressionResult.m_result != Compression::ResultCode::kSuccess)
                        {
                            Logger::LogError("Compression error");
                            blockResult = ResultCode::kDecompressionError;
                        }
                        else
                        {
                            FE_Assert(decompressionResult.m_decompressedSize == group->m_destinationSize);
                        }
                    }

                    SetOperationResult(operation, blockResult);

                    m_stagingAllocator.deallocate(group->m_stagingMemory, group->m_stagingMemorySize, Memory::kDefaultAlignment);
                    group->m_stagingMemory = nullptr;

                    CompleteOperationWork(operation);
                    m_groupPool.Delete(group);

                    // Notify the scheduler of freed memory.
                    m_queueEvent.Send();
                });
            }
        }

        if (!tg.IsEmpty())
            tg.Detach();
    }


    bool SchedulerImpl::TryFinalizeOperation(const Operation* operation)
    {
        if (operation->m_pendingWork.load(std::memory_order_acquire) > 0)
            return false;

        const ResultCode result = operation->m_controller->m_lastResult.load(std::memory_order_acquire);

        auto status = Status::kSucceeded;
        if (result == ResultCode::kCanceled)
            status = Status::kCanceled;
        else if (result != ResultCode::kSuccess)
            status = Status::kFailed;

        operation->m_controller->m_status.store(status, std::memory_order_release);

        if (const auto& callback = operation->m_batch.m_completionCallback)
            callback(operation->m_controller.Get());

        if (operation->m_batch.m_completionWaitGroup)
            operation->m_batch.m_completionWaitGroup->Signal();

        m_operationPool.Delete(operation);
        return true;
    }


    void SchedulerImpl::SchedulerThread()
    {
        for (;;)
        {
            m_queueEvent.Wait();

            for (;;)
            {
                ProcessBackendCompletions();

                Operation* operation = TryDequeue();
                if (operation)
                {
                    m_runningOperations.push_back(operation);
                    ProcessOperation(operation);
                }

                for (uint32_t operationIndex = 0; operationIndex < m_runningOperations.size();)
                {
                    if (TryFinalizeOperation(m_runningOperations[operationIndex]))
                    {
                        m_runningOperations.erase(m_runningOperations.begin() + operationIndex);
                    }
                    else
                    {
                        ++operationIndex;
                    }
                }

                m_fileCache.CollectGarbage();
                if (operation == nullptr)
                    break;
            }

            if (m_exitRequested)
                break;
        }
    }


    SchedulerImpl::SchedulerImpl()
    {
        m_stagingMemory = Memory::AllocateVirtual(kStagingHeapSize);
        m_stagingAllocator.Initialize(m_stagingMemory, kStagingHeapSize);

        m_queueEvent = Threading::Event::CreateAutoReset();

#if FE_PLATFORM_WINDOWS
        m_backend = Memory::DefaultNew<OverlappedAsyncIOBackend>(m_queueEvent);
#else
        m_backend = Memory::DefaultNew<DefaultAsyncIOBackend>();
#endif
        m_fileCache.Init(64, m_backend.Get());

        m_thread.Start("Async IO Scheduler", [this] {
            SchedulerThread();
        });
    }


    SchedulerImpl::~SchedulerImpl()
    {
        m_exitRequested = true;
        m_queueEvent.Send();
        m_thread.Join();

        m_fileCache.Shutdown();
        m_stagingAllocator.Shutdown();
        Memory::FreeVirtual(m_stagingMemory, kStagingHeapSize);
    }


    Rc<IController> SchedulerImpl::Read(const Batch& batch, const Priority priority)
    {
        Batch batchCopy(batch);
        return Read(std::move(batchCopy), priority);
    }


    Rc<IController> SchedulerImpl::Read(Batch&& batch, const Priority priority)
    {
        Rc controller = m_controllerPool.New();

        auto* operation = m_operationPool.New();
        operation->m_priority = priority;
        operation->m_batch = std::move(batch);
        operation->m_controller = controller;
        EnqueueImpl(operation);

        m_queueEvent.Send();
        return controller;
    }
} // namespace FE::IO::Async
