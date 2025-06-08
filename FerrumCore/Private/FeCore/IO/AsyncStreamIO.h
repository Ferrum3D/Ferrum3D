#pragma once
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Threading/Event.h>
#include <FeCore/Threading/Thread.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct AsyncController;


    struct AsyncRequestQueueEntry
    {
        enum class Type : uint32_t
        {
            kRead,
            kReadBlock,
            kCount,
        };

        Type m_type;
        Priority m_priority;
        std::atomic<bool> m_cancellationRequested = false;
        Rc<AsyncController> m_controller;
        std::atomic<AsyncOperationStatus> m_status = AsyncOperationStatus::kQueued;
        std::atomic<ResultCode> m_lastResult = ResultCode::kSuccess;
        AsyncOperationRequest* m_requestPtr = nullptr;
    };


    struct AsyncController final : public IAsyncController
    {
        AsyncRequestQueueEntry* m_requestEntry = nullptr;

        FE_RTTI_Class(AsyncController, "4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

        explicit AsyncController(AsyncRequestQueueEntry* entry)
            : m_requestEntry(entry)
        {
        }

        ~AsyncController() override = default;

        void Cancel() override
        {
            m_requestEntry->m_cancellationRequested.store(true, std::memory_order_release);
        }

        AsyncOperationStatus GetStatus() const override
        {
            return m_requestEntry->m_status.load(std::memory_order_acquire);
        }

        ResultCode GetLastOperationResult() const override
        {
            return m_requestEntry->m_lastResult.load(std::memory_order_acquire);
        }
    };


    struct AsyncReadRequestQueueEntry : public AsyncRequestQueueEntry
    {
        AsyncReadRequest m_request;
    };


    struct AsyncBlockReadRequestQueueEntry : public AsyncRequestQueueEntry
    {
        AsyncBlockReadRequest m_request;
        std::atomic<uint32_t> m_remainingBlockCount = 0;
    };


    struct AsyncStreamIO final : public IAsyncStreamIO
    {
        FE_RTTI_Class(AsyncStreamIO, "1ADBD843-E841-4B14-96EA-4AA08C901084");

        AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IStreamFactory* streamFactory);
        ~AsyncStreamIO() override;

        void ReadAsync(const AsyncReadRequest& request, Priority priority, IAsyncController** ppController) override;
        void ReadAsync(const AsyncBlockReadRequest& request, Priority priority, IAsyncController** ppController) override;

    private:
        Threading::ThreadHandle m_thread;
        Threading::Event m_queueEvent;
        Logger* m_logger = nullptr;
        std::atomic<bool> m_exitRequested;

        IJobSystem* m_jobSystem = nullptr;
        IStreamFactory* m_streamFactory = nullptr;

        TracyLockable(Threading::SpinLock, m_queueLock);
        festd::vector<AsyncRequestQueueEntry*> m_queue;

        Memory::SpinLockedPoolAllocator m_blockDecompressionJobPool;
        Memory::SpinLockedPoolAllocator m_requestPools[festd::to_underlying(AsyncRequestQueueEntry::Type::kCount)];
        Memory::SpinLockedPoolAllocator m_controllerPool{ "AsyncControllerPool", sizeof(AsyncController) };

        void EnqueueImpl(Priority priority, AsyncRequestQueueEntry* entry);

        AsyncRequestQueueEntry* TryDequeue();
        void ProcessGenericRequest(AsyncRequestQueueEntry* entry);

        void ProcessRequest(AsyncReadRequestQueueEntry* entry, AsyncOperationStatus status);
        void ProcessRequest(AsyncBlockReadRequestQueueEntry* entry, AsyncOperationStatus status);

        void ReaderThread();
    };
} // namespace FE::IO
