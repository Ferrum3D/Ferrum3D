#pragma once
#include <Core/IO/IAsyncStreamIO.h>
#include <Core/Logging/Logger.h>
#include <Core/Memory/LinearAllocator.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Event.h>
#include <Core/Threading/Thread.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct AsyncOperation;
    struct AsyncController;
    struct ReadGroup;


    struct AsyncReadHandle final : TypedHandle<AsyncReadHandle, uint32_t>
    {
    };


    struct AsyncIOPhysicalRead final
    {
        Path m_filePath;
        size_t m_offset = 0;
        void* m_destination = nullptr;
        size_t m_size = 0;
        ReadGroup* m_group = nullptr;
    };


    struct AsyncIOCompletion final
    {
        AsyncReadHandle m_handle;
        ReadGroup* m_group = nullptr;
        ResultCode m_result = ResultCode::kUnknownError;
        size_t m_bytesRead = 0;
    };


    struct IAsyncIOBackend : public Memory::RefCountedObjectBase
    {
        FE_RTTI("780D7B19-9084-4D59-9A45-A913994EFED6");

        ~IAsyncIOBackend() override = default;

        virtual AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) = 0;
        virtual bool PollRequestCompletion(AsyncIOCompletion& completion) = 0;
        virtual void Cancel(AsyncReadHandle handle) = 0;
        virtual void Tick() {}
    };


    struct DefaultAsyncIOBackend final : public IAsyncIOBackend
    {
        FE_RTTI("C1752D59-0343-46D0-B95A-127EB2321CC7");

        AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) override;
        bool PollRequestCompletion(AsyncIOCompletion& completion) override;
        void Cancel(AsyncReadHandle handle) override;

    private:
        struct CompletedRead final
        {
            AsyncReadHandle m_handle;
            ReadGroup* m_group = nullptr;
            ResultCode m_result = ResultCode::kUnknownError;
            size_t m_bytesRead = 0;
        };

        uint32_t m_nextHandle = 1;
        festd::vector<CompletedRead> m_completions;
    };


#if FE_PLATFORM_WINDOWS
    struct OverlappedAsyncIOBackend final : public IAsyncIOBackend
    {
        FE_RTTI("70064B01-C464-4AD7-8169-C61C9D37419A");

        OverlappedAsyncIOBackend();
        ~OverlappedAsyncIOBackend() override;

        AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) override;
        bool PollRequestCompletion(AsyncIOCompletion& completion) override;
        void Cancel(AsyncReadHandle handle) override;
        void Tick() override;

    private:
        struct PendingRead;

        AsyncReadHandle StartRead(PendingRead* read);
        void CloseRead(PendingRead* read);
        PendingRead* FindRead(AsyncReadHandle handle);
        void DispatchQueuedReads();

        void* m_completionPort = nullptr;
        uint32_t m_nextHandle = 1;
        uint32_t m_outstandingReadCount = 0;
        festd::vector<PendingRead*> m_reads;
        festd::vector<PendingRead*> m_queuedReads;
    };
#endif


    struct AsyncOperation final
    {
        Priority m_priority = Priority::kNormal;
        std::atomic<uint32_t> m_pendingWork = 0;
        Rc<AsyncController> m_controller;
        AsyncReadCommandList m_commandList;
        Memory::SpinLockedPoolAllocator* m_operationPool = nullptr;
        Memory::SpinLockedPoolAllocator* m_controllerPool = nullptr;
        Memory::SpinLockedPoolAllocator* m_groupPool = nullptr;
        Memory::SpinLockedPoolAllocator* m_decompressionJobPool = nullptr;
        std::atomic<bool> m_completionRequested = false;
        std::atomic<bool> m_completed = false;
        Threading::SpinLock m_completionLock;
        festd::vector<InternalAsyncReadCommands::AsyncInvokeFunctorCommand> m_completionCallbacks;
    };


    struct AsyncController final : public IAsyncController
    {
        FE_RTTI("4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

        AsyncController() = default;
        ~AsyncController() override = default;

        void Cancel() override;
        AsyncOperationStatus GetStatus() const override;
        ResultCode GetLastOperationResult() const override;

        std::atomic<bool> m_cancellationRequested = false;
        std::atomic<AsyncOperationStatus> m_status = AsyncOperationStatus::kQueued;
        std::atomic<ResultCode> m_lastResult = ResultCode::kSuccess;
    };


    struct AsyncStreamIO final : public IAsyncStreamIO
    {
        FE_RTTI("1ADBD843-E841-4B14-96EA-4AA08C901084");

        AsyncStreamIO(Logger* logger, IJobSystem* jobSystem, IAsyncIOBackend* backend);
        ~AsyncStreamIO() override;

        void ExecuteCommandList(AsyncReadCommandList* commandList, Priority priority, IAsyncController** ppController) override;

    private:
        Threading::ThreadHandle m_thread;
        Threading::Event m_queueEvent;
        Logger* m_logger = nullptr;
        std::atomic<bool> m_exitRequested = false;

        IJobSystem* m_jobSystem = nullptr;
        Rc<IAsyncIOBackend> m_backend;

        TracyLockable(Threading::SpinLock, m_queueLock);
        festd::vector<AsyncOperation*> m_queue;
        festd::vector<AsyncOperation*> m_runningOperations;

        Memory::SpinLockedPoolAllocator m_operationPool{ "IO/Async/OperationPool", sizeof(AsyncOperation) };
        Memory::SpinLockedPoolAllocator m_controllerPool{ "IO/Async/ControllerPool", sizeof(AsyncController) };
        Memory::SpinLockedPoolAllocator m_groupPool;
        Memory::SpinLockedPoolAllocator m_decompressionJobPool;
        Memory::TLSFAllocator m_stagingAllocator;

        void EnqueueImpl(AsyncOperation* operation);
        AsyncOperation* TryDequeue();
        void ProcessCommandList(AsyncOperation* operation);
        void ProcessBackendCompletions();
        void RequestOperationCompletion(AsyncOperation* operation, ResultCode result);
        bool TryFinalizeOperation(AsyncOperation* operation);
        void ReaderThread();
    };
} // namespace FE::IO
