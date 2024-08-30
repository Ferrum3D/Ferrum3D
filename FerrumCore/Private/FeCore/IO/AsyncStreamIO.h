#pragma once
#include <FeCore/EventBus/CoreEvents.h>
#include <FeCore/IO/IAsyncStreamIO.h>
#include <FeCore/Logging/Logger.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Parallel/Event.h>
#include <FeCore/Parallel/Thread.h>

namespace FE::IO
{
    struct AsyncReadRequestQueueEntry final
    {
        Rc<IAsyncController> pController;
        AsyncReadRequest Request;
        std::atomic<AsyncOperationStatus> Status = AsyncOperationStatus::kQueued;
        std::atomic<ResultCode> LastResult = ResultCode::Success;
        std::atomic<bool> CancellationRequested = false;
    };


    class AsyncStreamIO final : public IAsyncStreamIO
    {
        ThreadHandle m_Thread;
        Threading::Event m_QueueEvent;
        Logger* m_pLogger = nullptr;
        std::atomic<bool> m_ExitRequested;

        IStreamFactory* m_pStreamFactory = nullptr;

        TracyLockable(SpinLock, m_QueueLock);
        Memory::PoolAllocator m_RequestPool;
        Memory::PoolAllocator m_ControllerPool;
        festd::vector<AsyncReadRequestQueueEntry*> m_Queue;

        AsyncReadRequestQueueEntry* TryDequeue();
        void ProcessRequest(AsyncReadRequestQueueEntry* entry);

        void ReaderThread();

    public:
        FE_RTTI_Class(AsyncStreamIO, "1ADBD843-E841-4B14-96EA-4AA08C901084");

        AsyncStreamIO(Logger* pLogger, IStreamFactory* pStreamFactory);
        ~AsyncStreamIO() override;

        void ReadAsync(const AsyncReadRequest& request, IAsyncController** ppController) override;
    };
} // namespace FE::IO
