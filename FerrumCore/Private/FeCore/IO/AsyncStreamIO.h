#pragma once
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Parallel/Event.h>
#include <FeCore/Parallel/Thread.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct AsyncReadRequestQueueEntry final
    {
        Rc<IAsyncController> m_pController;
        AsyncReadRequest m_request;
        std::atomic<AsyncOperationStatus> m_status = AsyncOperationStatus::kQueued;
        std::atomic<ResultCode> m_lastResult = ResultCode::Success;
        std::atomic<bool> m_cancellationRequested = false;
    };


    struct AsyncStreamIO final : public IAsyncStreamIO
    {
        FE_RTTI_Class(AsyncStreamIO, "1ADBD843-E841-4B14-96EA-4AA08C901084");

        AsyncStreamIO(Logger* pLogger, IStreamFactory* pStreamFactory);
        ~AsyncStreamIO() override;

        void ReadAsync(const AsyncReadRequest& request, IAsyncController** ppController) override;

    private:
        ThreadHandle m_thread;
        Threading::Event m_queueEvent;
        Logger* m_logger = nullptr;
        std::atomic<bool> m_exitRequested;

        IStreamFactory* m_streamFactory = nullptr;

        TracyLockable(Threading::SpinLock, m_queueLock);
        Memory::PoolAllocator m_requestPool;
        Memory::PoolAllocator m_controllerPool;
        festd::vector<AsyncReadRequestQueueEntry*> m_queue;

        AsyncReadRequestQueueEntry* TryDequeue();
        void ProcessRequest(AsyncReadRequestQueueEntry* entry);

        void ReaderThread();
    };
} // namespace FE::IO
