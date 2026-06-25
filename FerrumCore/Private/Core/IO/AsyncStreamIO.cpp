#include <Core/IO/AsyncStreamIO.h>
#include <Core/Jobs/Job.h>
#include <Core/Logging/Trace.h>
#include <Core/Memory/Memory.h>

namespace FE::IO
{
    struct ReadGroup final
    {
        AsyncOperation* m_operation = nullptr;
        AsyncReadHandle m_handle;
        void* m_stagingMemory = nullptr;
        size_t m_stagingMemorySize = 0;
        std::byte* m_destination = nullptr;
        size_t m_destinationSize = 0;
        size_t m_readSize = 0;
        Compression::Method m_compressionMethod = Compression::Method::kNone;
        JobPriority m_decompressionPriority = JobPriority::kNormal;
    };


    namespace
    {
        constexpr uint32_t kSuccessColor = 0x4b4e6d;
        constexpr uint32_t kFailureColor = 0x9a031e;
        constexpr size_t kStagingHeapSize = 256 * 1024 * 1024;


        void SetOperationResult(AsyncOperation* operation, const ResultCode result)
        {
            if (result == ResultCode::kSuccess)
                return;

            ResultCode expected = ResultCode::kSuccess;
            operation->m_controller->m_lastResult.compare_exchange_strong(expected, result, std::memory_order_acq_rel);
        }


        void CompleteOperationWork(AsyncOperation* operation)
        {
            const uint32_t previousValue = operation->m_pendingWork.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previousValue > 0, "Async operation pending work underflow");
        }


        void FreeReadGroup(ReadGroup* group)
        {
            AsyncOperation* operation = group->m_operation;
            if (group->m_stagingMemory)
                group->m_stagingMemory = nullptr;

            Memory::Delete(operation->m_groupPool, group);
        }


        struct DecompressionJob final : public Job
        {
            void Execute() override
            {
                FE_PROFILER_ZONE();

                AsyncOperation* operation = m_group->m_operation;
                ResultCode result = ResultCode::kSuccess;

                if (operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire))
                {
                    result = ResultCode::kCanceled;
                }
                else
                {
                    const auto decompressor = Compression::Decompressor::Create(m_group->m_compressionMethod);
                    const Compression::DecompressionResult decompressionResult =
                        decompressor.Decompress(m_group->m_stagingMemory,
                                                m_group->m_stagingMemorySize,
                                                m_group->m_destination,
                                                m_group->m_destinationSize);

                    if (decompressionResult.m_result != Compression::ResultCode::kSuccess
                        || decompressionResult.m_decompressedSize != m_group->m_destinationSize)
                    {
                        result = ResultCode::kDecompressionError;
                    }
                }

                SetOperationResult(operation, result);
                m_stagingAllocator->deallocate(m_group->m_stagingMemory, m_group->m_stagingMemorySize, Memory::kDefaultAlignment);
                m_group->m_stagingMemory = nullptr;
                CompleteOperationWork(operation);
                operation->m_completionRequested.store(true, std::memory_order_release);
                FreeReadGroup(m_group);
                m_wakeEvent->Send();
                Memory::Delete(operation->m_decompressionJobPool, this);
            }

            ReadGroup* m_group = nullptr;
            std::pmr::memory_resource* m_stagingAllocator = nullptr;
            Threading::Event* m_wakeEvent = nullptr;
        };


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
                                           const size_t compressedSize, const Compression::Method compressionMethod,
                                           const JobPriority decompressionPriority)
    {
        InternalAsyncReadCommands::AsyncReadCompressedCommand command;
        command.m_destination = destination;
        command.m_destinationSize = destinationSize;
        command.m_sourceOffset = sourceOffset;
        command.m_compressedSize = compressedSize;
        command.m_compressionMethod = compressionMethod;
        command.m_decompressionPriority = decompressionPriority;
        m_bufferBuilder.WriteBytes(&command, sizeof(command));
    }


    AsyncReadCommandList AsyncReadCommandListBuilder::Build(WaitGroup* signalWaitGroup)
    {
        AsyncReadCommandList commandList;
        commandList.m_buffer = m_bufferBuilder.Build();
        commandList.m_signalWaitGroup = signalWaitGroup;
        commandList.m_allocator = nullptr;
        return commandList;
    }


    void AsyncController::Cancel()
    {
        m_cancellationRequested.store(true, std::memory_order_release);
    }


    AsyncOperationStatus AsyncController::GetStatus() const
    {
        return m_status.load(std::memory_order_acquire);
    }


    ResultCode AsyncController::GetLastOperationResult() const
    {
        return m_lastResult.load(std::memory_order_acquire);
    }


    void AsyncStreamIO::EnqueueImpl(AsyncOperation* operation)
    {
        const auto iter = festd::upper_bound(m_queue, operation->m_priority, [](const Priority lhs, const AsyncOperation* rhs) {
            return lhs < rhs->m_priority;
        });

        m_queue.insert(iter, operation);
    }


    AsyncOperation* AsyncStreamIO::TryDequeue()
    {
        if (m_queue.empty())
            return nullptr;

        AsyncOperation* operation = m_queue.front();
        m_queue.erase(m_queue.begin());
        return operation;
    }


    void AsyncStreamIO::ProcessCommandList(AsyncOperation* operation)
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
                    operation->m_completionCallbacks.push_back(command);
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
                        break;
                    }

                    auto* group = Memory::New<ReadGroup>(operation->m_groupPool);
                    group->m_operation = operation;
                    group->m_destination = command.m_destination;
                    group->m_destinationSize = command.m_destinationSize;
                    group->m_readSize = command.m_destinationSize;

                    operation->m_pendingWork.fetch_add(1, std::memory_order_acq_rel);
                    AsyncIOPhysicalRead read;
                    read.m_filePath = currentSource.m_filePath;
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
                        break;
                    }

                    auto* group = Memory::New<ReadGroup>(operation->m_groupPool);
                    group->m_operation = operation;
                    group->m_destination = command.m_destination;
                    group->m_destinationSize = command.m_destinationSize;
                    group->m_readSize = command.m_compressedSize;
                    group->m_stagingMemorySize = command.m_compressedSize;
                    group->m_stagingMemory = m_stagingAllocator.allocate(command.m_compressedSize, Memory::kDefaultAlignment);
                    group->m_compressionMethod = command.m_compressionMethod;
                    group->m_decompressionPriority = command.m_decompressionPriority;

                    operation->m_pendingWork.fetch_add(1, std::memory_order_acq_rel);
                    AsyncIOPhysicalRead read;
                    read.m_filePath = currentSource.m_filePath;
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
        AsyncIOCompletion completion;
        while (m_backend->PollRequestCompletion(completion))
        {
            ReadGroup* group = completion.m_group;
            if (group == nullptr)
                continue;

            AsyncOperation* operation = group->m_operation;
            ResultCode result = completion.m_result;
            if (result == ResultCode::kSuccess && completion.m_bytesRead != group->m_readSize)
                result = ResultCode::kIOError;

            if (operation->m_controller->m_cancellationRequested.load(std::memory_order_acquire))
                result = ResultCode::kCanceled;

            if (result != ResultCode::kSuccess)
            {
                SetOperationResult(operation, result);
                if (group->m_stagingMemory)
                {
                    m_stagingAllocator.deallocate(group->m_stagingMemory, group->m_stagingMemorySize, Memory::kDefaultAlignment);
                    group->m_stagingMemory = nullptr;
                }

                CompleteOperationWork(operation);
                FreeReadGroup(group);
                continue;
            }

            if (group->m_compressionMethod != Compression::Method::kNone)
            {
                auto* job = Memory::New<DecompressionJob>(operation->m_decompressionJobPool);
                job->m_group = group;
                job->m_stagingAllocator = &m_stagingAllocator;
                job->m_wakeEvent = &m_queueEvent;
                job->ScheduleBackground(m_jobSystem, nullptr, group->m_decompressionPriority);
                continue;
            }

            CompleteOperationWork(operation);
            FreeReadGroup(group);
        }
    }


    void AsyncStreamIO::RequestOperationCompletion(AsyncOperation* operation, const ResultCode result)
    {
        SetOperationResult(operation, result);
        operation->m_completionRequested.store(true, std::memory_order_release);
    }


    bool AsyncStreamIO::TryFinalizeOperation(AsyncOperation* operation)
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

        for (const InternalAsyncReadCommands::AsyncInvokeFunctorCommand& command : operation->m_completionCallbacks)
            command.m_functor(command.m_context);

        if (operation->m_commandList.m_signalWaitGroup)
            operation->m_commandList.m_signalWaitGroup->Signal();

        operation->m_commandList.m_buffer.Free();

        Memory::Delete(operation->m_operationPool, operation);
        return true;
    }


    void AsyncStreamIO::ReaderThread()
    {
        for (;;)
        {
            m_queueEvent.Wait();

            if (m_exitRequested)
                break;

            bool anyProgress = false;

            for (;;)
            {
                m_backend->Tick();
                ProcessBackendCompletions();

                AsyncOperation* operation = nullptr;
                {
                    std::lock_guard lk{ m_queueLock };
                    operation = TryDequeue();
                }

                if (operation)
                {
                    m_runningOperations.push_back(operation);
                    ProcessCommandList(operation);
                    anyProgress = true;
                }

                for (uint32_t index = 0; index < m_runningOperations.size();)
                {
                    AsyncOperation* runningOperation = m_runningOperations[index];
                    const bool finalized = TryFinalizeOperation(runningOperation);
                    if (finalized)
                    {
                        m_runningOperations.erase(m_runningOperations.begin() + index);
                        anyProgress = true;
                    }
                    else
                    {
                        ++index;
                    }
                }

                if (m_exitRequested)
                    return;

                bool queueEmpty;
                {
                    std::lock_guard lk{ m_queueLock };
                    queueEmpty = m_queue.empty();
                    if (queueEmpty && m_runningOperations.empty())
                    {
                        m_queueEvent.Reset();
                        if (!m_queue.empty())
                            m_queueEvent.Send();
                        break;
                    }
                }

                if (!anyProgress)
                    Threading::Sleep(1);

                anyProgress = false;
            }
        }
    }


    AsyncStreamIO::AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IAsyncIOBackend* backend)
        : m_logger(logger)
        , m_jobSystem(jobSystem)
        , m_backend(backend)
        , m_stagingAllocator(Memory::AllocateVirtual(kStagingHeapSize), kStagingHeapSize)
    {
        m_groupPool.Initialize("IO/Async/ReadGroupPool", sizeof(ReadGroup));
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


    void AsyncStreamIO::ExecuteCommandList(AsyncReadCommandList* commandList, const Priority priority,
                                           IAsyncController** ppController)
    {
        auto* operation = Memory::New<AsyncOperation>(&m_operationPool);
        auto* controller = Rc<AsyncController>::New(&m_controllerPool);
        operation->m_priority = priority;
        operation->m_commandList = *commandList;
        operation->m_controller = controller;
        operation->m_operationPool = &m_operationPool;
        operation->m_controllerPool = &m_controllerPool;
        operation->m_groupPool = &m_groupPool;
        operation->m_decompressionJobPool = &m_decompressionJobPool;

        if (commandList->m_allocator)
        {
            Memory::Delete(commandList->m_allocator, commandList);
            operation->m_commandList.m_allocator = nullptr;
        }

        {
            std::lock_guard lk{ m_queueLock };
            EnqueueImpl(operation);
        }

        if (ppController)
        {
            controller->AddRef();
            *ppController = controller;
        }

        m_queueEvent.Send();
    }
} // namespace FE::IO
