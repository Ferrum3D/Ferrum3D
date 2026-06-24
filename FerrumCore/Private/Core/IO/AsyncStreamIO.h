#pragma once
#include <Core/IO/IAsyncStreamIO.h>
#include <Core/Logging/Logger.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Event.h>
#include <Core/Threading/Thread.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct AsyncController;


    struct AsyncRequestQueueEntry final
    {
        Priority m_priority;
        std::atomic<bool> m_cancellationRequested = false;
        Rc<AsyncController> m_controller;
        std::atomic<AsyncOperationStatus> m_status = AsyncOperationStatus::kQueued;
        std::atomic<ResultCode> m_lastResult = ResultCode::kSuccess;
        AsyncReadRequest m_request;
    };


    struct AsyncController final : public IAsyncController
    {
        AsyncRequestQueueEntry* m_requestEntry = nullptr;

        FE_RTTI("4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

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


    struct AsyncStreamIO final : public IAsyncStreamIO
    {
        FE_RTTI("1ADBD843-E841-4B14-96EA-4AA08C901084");

        AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IStreamFactory* streamFactory);
        ~AsyncStreamIO() override;

        void ReadAsync(const AsyncReadRequest& request, Priority priority, IAsyncController** ppController) override;

    private:
        Threading::ThreadHandle m_thread;
        Threading::Event m_queueEvent;
        Logger* m_logger = nullptr;
        std::atomic<bool> m_exitRequested = false;

        IJobSystem* m_jobSystem = nullptr;
        IStreamFactory* m_streamFactory = nullptr;

        TracyLockable(Threading::SpinLock, m_queueLock);
        festd::vector<AsyncRequestQueueEntry*> m_queue;

        Memory::SpinLockedPoolAllocator m_decompressionJobPool;
        Memory::SpinLockedPoolAllocator m_requestPool{ "IO/Async/ReadRequestPool", sizeof(AsyncRequestQueueEntry) };
        Memory::SpinLockedPoolAllocator m_controllerPool{ "IO/Async/ControllerPool", sizeof(AsyncController) };

        void EnqueueImpl(Priority priority, AsyncRequestQueueEntry* entry);

        AsyncRequestQueueEntry* TryDequeue();
        void ProcessRequest(AsyncRequestQueueEntry* entry, AsyncOperationStatus status);

        void ReaderThread();
    };
} // namespace FE::IO
