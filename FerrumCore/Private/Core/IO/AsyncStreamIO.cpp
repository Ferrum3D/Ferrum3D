#include <Core/IO/AsyncStreamIO.h>
#include <Core/IO/Platform/PlatformFile.h>
#include <Core/Jobs/Job.h>
#include <Core/Jobs/TaskGraph.h>
#include <Core/Logging/Trace.h>
#include <Core/Memory/Memory.h>

namespace FE::IO
{
    namespace
    {
        constexpr uint32_t kSuccessColor = 0x4b4e6d;
        constexpr uint32_t kFailureColor = 0x9a031e;
        constexpr size_t kStagingHeapSize = 256 * 1024 * 1024;


        void SetOperationResult(AsyncIOOperation* operation, const ResultCode result)
        {
            if (result == ResultCode::kSuccess)
                return;

            auto expected = ResultCode::kSuccess;
            operation->m_controller->m_lastResult.compare_exchange_strong(expected, result, std::memory_order_acq_rel);
        }


        void CompleteOperationWork(AsyncIOOperation* operation)
        {
            const uint32_t previousValue = operation->m_pendingWork.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previousValue > 0, "Async operation pending work underflow");
        }


        void FreeReadGroup(Memory::SpinLockedPool<ReadGroup>& pool, ReadGroup* group)
        {
            if (group->m_stagingMemory)
                group->m_stagingMemory = nullptr;

            pool.Delete(group);
        }


        bool IsReadInsideSource(const ResolvedDataSource& source, const size_t sourceOffset, const size_t byteSize)
        {
            if (source.m_filePath.empty() || byteSize == 0)
                return false;

            if (source.m_byteSize == 0)
                return true;

            return sourceOffset <= source.m_byteSize && byteSize <= source.m_byteSize - sourceOffset;
        }

    } // namespace


    void* AsyncReadCommandListBuilder::Allocate(const size_t bytes, const size_t alignment)
    {
        const uint32_t alignedBytes = AlignUp<uint32_t>(static_cast<uint32_t>(bytes), static_cast<uint32_t>(alignment));
        return m_bufferBuilder.Allocate(alignedBytes);
    }


    void AsyncReadCommandListBuilder::SetSource(const ResolvedDataSource& source)
    {
        InternalAsyncReadCommands::AsyncSetSourceCommand command;
        command.m_source = source;
        m_bufferBuilder.WriteBytes(&command, sizeof(command));
    }


    void AsyncReadCommandListBuilder::Read(std::byte* destination, const size_t destinationSize, const size_t sourceOffset)
    {
        InternalAsyncReadCommands::AsyncReadCommand command;
        command.m_destination = destination;
        command.m_destinationSize = destinationSize;
        command.m_sourceOffset = sourceOffset;
        m_bufferBuilder.WriteBytes(&command, sizeof(command));
    }


    void AsyncReadCommandListBuilder::Read(std::byte* destination, const size_t destinationSize, const size_t sourceOffset,
                                           const size_t compressedSize, const Compression::Method compressionMethod)
    {
        InternalAsyncReadCommands::AsyncReadCompressedCommand command;
        command.m_destination = destination;
        command.m_destinationSize = destinationSize;
        command.m_sourceOffset = sourceOffset;
        command.m_compressedSize = compressedSize;
        command.m_compressionMethod = compressionMethod;
        m_bufferBuilder.WriteBytes(&command, sizeof(command));
    }


    AsyncReadCommandList AsyncReadCommandListBuilder::Build(WaitGroup* signalWaitGroup)
    {
        AsyncReadCommandList commandList;
        commandList.m_buffer = m_bufferBuilder.Build();
        commandList.m_signalWaitGroup = signalWaitGroup;
        return commandList;
    }


    void AsyncIOController::Cancel()
    {
        m_cancellationRequested.store(true, std::memory_order_release);
    }


    AsyncOperationStatus AsyncIOController::GetStatus() const
    {
        return m_status.load(std::memory_order_acquire);
    }


    ResultCode AsyncIOController::GetLastOperationResult() const
    {
        return m_lastResult.load(std::memory_order_acquire);
    }


    void AsyncIOOpenFileCache::Init(const uint32_t cacheSize, IAsyncIOBackend* backend)
    {
        m_backend = backend;
        m_cacheSize = cacheSize;
        m_entries.reserve(cacheSize);
    }


    festd::expected<Rc<AsyncIOCachedFile>, ResultCode> AsyncIOOpenFileCache::CreateFile(const festd::string_view path)
    {
        std::unique_lock lk{ m_lock };

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

        auto newEntry = Rc<AsyncIOCachedFile>::New(&m_entryPool);
        newEntry->m_nameHash = hash;
        newEntry->m_path = path;
        newEntry->m_fileHandle = openFileResult.value();
        m_entries.push_back(newEntry);
        return newEntry;
    }


    void AsyncIOOpenFileCache::CollectGarbage()
    {
        const uint64_t timestamp = Platform::GetTicks();
        const double ticksPerSecond = Platform::GetSecondsPerTick();
        for (uint32_t entryIndex = 0; entryIndex < m_entries.size(); ++entryIndex)
        {
            if (m_entries[entryIndex]->GetRefCount() == 1) // cache is the only owner
            {
                const Rc<AsyncIOCachedFile> entry = m_entries[entryIndex];
                const double elapsedSeconds = static_cast<double>(timestamp - entry->m_lastUseTime) * ticksPerSecond;
                if (elapsedSeconds > 20)
                {
                    DeleteEntry(entryIndex);
                    --entryIndex;
                }
            }
        }
    }


    void AsyncIOOpenFileCache::DeleteEntry(const uint32_t entryIndex)
    {
        Rc<AsyncIOCachedFile> entry = m_entries[entryIndex];
        m_entries.erase(m_entries.begin() + entryIndex);
        Platform::CloseFile(entry->m_fileHandle);
        entry->m_fileHandle.Reset();
    }


    void AsyncStreamIO::EnqueueImpl(AsyncIOOperation* operation)
    {
        const auto iter = festd::upper_bound(m_queue, operation->m_priority, [](const Priority lhs, const AsyncIOOperation* rhs) {
            return lhs < rhs->m_priority;
        });

        m_queue.insert(iter, operation);
    }


    AsyncIOOperation* AsyncStreamIO::TryDequeue()
    {
        std::unique_lock lk{ m_queueLock };
        if (m_queue.empty())
            return nullptr;

        AsyncIOOperation* operation = m_queue.front();
        m_queue.erase(m_queue.begin());
        return operation;
    }


    void AsyncStreamIO::ProcessCommandList(AsyncIOOperation* operation)
    {
        FE_PROFILER_ZONE_NAMED("AsyncReadCommandList");

        operation->m_controller->m_status.store(AsyncOperationStatus::kRunning, std::memory_order_release);

        const bool canceledOnStart = operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire);
        if (canceledOnStart)
            SetOperationResult(operation, ResultCode::kCanceled);

        ResolvedDataSource currentSource;
        Memory::SegmentedBufferReader reader{ operation->m_commandList.m_buffer };
        for (;;)
        {
            using namespace InternalAsyncReadCommands;

            AsyncReadCommandType commandType;
            if (!reader.ReadNoConsume(commandType))
                break;

            switch (commandType)
            {
            case AsyncReadCommandType::kSetSource:
                {
                    AsyncSetSourceCommand command;
                    FE_Verify(reader.Read(command));
                    currentSource = command.m_source;
                    break;
                }

            case AsyncReadCommandType::kInvokeFunctor:
                {
                    AsyncInvokeFunctorCommand command;
                    FE_Verify(reader.Read(command));
                    operation->m_completionCallback = command;
                    FE_Verify(reader.SkipBytes(command.m_functorSize));
                    break;
                }

            case AsyncReadCommandType::kRead:
                {
                    AsyncReadCommand command;
                    FE_Verify(reader.Read(command));
                    if (canceledOnStart)
                        break;

                    if (!IsReadInsideSource(currentSource, command.m_sourceOffset, command.m_destinationSize)
                        || command.m_destination == nullptr)
                    {
                        RequestOperationCompletion(operation, ResultCode::kInvalidArgument);
                        FE_DebugBreak();
                        break;
                    }

                    const festd::expected fileOpenResult = m_fileCache.CreateFile(currentSource.m_filePath);
                    if (!fileOpenResult.has_value())
                    {
                        RequestOperationCompletion(operation, fileOpenResult.error());
                        break;
                    }

                    auto* group = m_groupPool.New();
                    group->m_operation = operation;
                    group->m_destination = command.m_destination;
                    group->m_destinationSize = command.m_destinationSize;
                    group->m_readSize = command.m_destinationSize;
                    operation->m_pendingWork.fetch_add(1, std::memory_order_acq_rel);

                    AsyncIOPhysicalRead read;
                    read.m_file = fileOpenResult.value();
                    read.m_offset = currentSource.m_byteOffset + command.m_sourceOffset;
                    read.m_destination = command.m_destination;
                    read.m_size = command.m_destinationSize;
                    read.m_group = group;
                    group->m_handle = m_backend->DispatchRead(read);
                    break;
                }

            case AsyncReadCommandType::kReadCompressed:
                {
                    AsyncReadCompressedCommand command;
                    FE_Verify(reader.Read(command));
                    if (canceledOnStart)
                        break;

                    if (!IsReadInsideSource(currentSource, command.m_sourceOffset, command.m_compressedSize)
                        || command.m_destination == nullptr || command.m_compressedSize == 0 || command.m_destinationSize == 0
                        || command.m_compressionMethod == Compression::Method::kNone
                        || command.m_compressionMethod >= Compression::Method::kInvalid)
                    {
                        RequestOperationCompletion(operation, ResultCode::kInvalidArgument);
                        FE_DebugBreak();
                        break;
                    }

                    const festd::expected fileOpenResult = m_fileCache.CreateFile(currentSource.m_filePath);
                    if (!fileOpenResult.has_value())
                    {
                        RequestOperationCompletion(operation, fileOpenResult.error());
                        break;
                    }

                    auto* group = m_groupPool.New();
                    group->m_operation = operation;
                    group->m_destination = command.m_destination;
                    group->m_destinationSize = command.m_destinationSize;
                    group->m_readSize = command.m_compressedSize;
                    group->m_stagingMemorySize = command.m_compressedSize;
                    group->m_stagingMemory = m_stagingAllocator.allocate(command.m_compressedSize, Memory::kDefaultAlignment);
                    group->m_compressionMethod = command.m_compressionMethod;
                    operation->m_pendingWork.fetch_add(1, std::memory_order_acq_rel);

                    AsyncIOPhysicalRead read;
                    read.m_file = fileOpenResult.value();
                    read.m_offset = currentSource.m_byteOffset + command.m_sourceOffset;
                    read.m_destination = group->m_stagingMemory;
                    read.m_size = command.m_compressedSize;
                    read.m_group = group;
                    group->m_handle = m_backend->DispatchRead(read);
                    break;
                }

            case AsyncReadCommandType::kInvalid:
            default:
                RequestOperationCompletion(operation, ResultCode::kInvalidArgument);
                FE_DebugBreak();
                break;
            }
        }

        operation->m_completionRequested.store(true, std::memory_order_release);
    }


    void AsyncStreamIO::ProcessBackendCompletions()
    {
        TaskGraph tg{ "IO/Async/RequestGraph", m_jobSystem, FiberAffinityMask::kAllBackground };

        AsyncIOCompletion completion;
        while (m_backend->PollRequestCompletion(completion))
        {
            ReadGroup* group = completion.m_group;
            if (group == nullptr)
                continue;

            AsyncIOOperation* operation = group->m_operation;
            ResultCode result = completion.m_result;
            if (result == ResultCode::kSuccess && completion.m_bytesRead != group->m_readSize)
                FE_DebugBreak();

            if (operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire))
                result = ResultCode::kCanceled;

            if (result != ResultCode::kSuccess)
            {
                SetOperationResult(operation, result);
                FE_Assert(group->m_stagingMemory == nullptr);

                CompleteOperationWork(operation);
                FreeReadGroup(m_groupPool, group);
                continue;
            }

            if (group->m_compressionMethod == Compression::Method::kNone)
            {
                CompleteOperationWork(operation);
                FreeReadGroup(m_groupPool, group);
            }
            else
            {
                tg.Schedule("Block", [this, operation, group] {
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
                            m_logger->LogError("Compression error");
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
                    operation->m_completionRequested.store(true, std::memory_order_release);

                    FreeReadGroup(m_groupPool, group);

                    // Notify the scheduler of freed memory.
                    m_queueEvent.Send();
                });
            }
        }

        tg.Detach();
    }


    void AsyncStreamIO::RequestOperationCompletion(AsyncIOOperation* operation, const ResultCode result)
    {
        SetOperationResult(operation, result);
        operation->m_completionRequested.store(true, std::memory_order_release);
    }


    bool AsyncStreamIO::TryFinalizeOperation(AsyncIOOperation* operation)
    {
        if (!operation->m_completionRequested.load(std::memory_order_acquire)
            || operation->m_pendingWork.load(std::memory_order_acquire) != 0
            || operation->m_completed.exchange(true, std::memory_order_acq_rel))
        {
            return false;
        }

        const ResultCode result = operation->m_controller->m_lastResult.load(std::memory_order_acquire);
        AsyncOperationStatus status = AsyncOperationStatus::kSucceeded;
        if (result == ResultCode::kCanceled)
            status = AsyncOperationStatus::kCanceled;
        else if (result != ResultCode::kSuccess)
            status = AsyncOperationStatus::kFailed;

        operation->m_controller->m_status.store(status, std::memory_order_release);

        if (const auto& command = operation->m_completionCallback; command.m_functor)
        {
            FE_AssertDebug(command.m_type == InternalAsyncReadCommands::AsyncReadCommandType::kInvokeFunctor);
            FE_AssertDebug(command.m_context != nullptr);
            command.m_functor(command.m_context);
        }

        if (operation->m_commandList.m_signalWaitGroup)
            operation->m_commandList.m_signalWaitGroup->Signal();

        operation->m_commandList.m_buffer.Free();

        Memory::Delete(&m_operationPool, operation);
        return true;
    }


    void AsyncStreamIO::SchedulerThread()
    {
        for (;;)
        {
            m_queueEvent.Wait();

            for (;;)
            {
                ProcessBackendCompletions();

                AsyncIOOperation* operation = TryDequeue();
                if (operation)
                {
                    m_runningOperations.push_back(operation);
                    ProcessCommandList(operation);
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

                if (operation == nullptr)
                    break;
            }

            if (m_exitRequested)
                break;
        }
    }


    AsyncStreamIO::AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IAsyncIOBackend* backend)
        : m_logger(logger)
        , m_jobSystem(jobSystem)
        , m_backend(backend)
        , m_stagingAllocator(Memory::AllocateVirtual(kStagingHeapSize), kStagingHeapSize)
    {
        m_fileCache.Init(64, backend);

        m_queueEvent = Threading::Event::CreateAutoReset();
        m_thread.Start("Async IO Scheduler", [this] {
            SchedulerThread();
        });
    }


    AsyncStreamIO::~AsyncStreamIO()
    {
        m_exitRequested = true;
        m_queueEvent.Send();
    }


    Rc<IAsyncController> AsyncStreamIO::ExecuteCommandList(const AsyncReadCommandList& commandList, const Priority priority)
    {
        Rc controller = Rc<AsyncIOController>::New(&m_controllerPool);

        auto* operation = m_operationPool.New();
        operation->m_priority = priority;
        operation->m_commandList = commandList;
        operation->m_controller = controller;

        {
            std::unique_lock lk{ m_queueLock };
            EnqueueImpl(operation);
        }

        m_queueEvent.Send();
        return controller;
    }
} // namespace FE::IO
