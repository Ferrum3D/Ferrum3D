#pragma once
#include <Core/IO/IAsyncStreamIO.h>
#include <Core/Logging/Logger.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Event.h>
#include <Core/Threading/Thread.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct AsyncIOOperation;
    struct AsyncIOController;
    struct AsyncIOCachedFile;
    struct AsyncIOOpenFileCache;


    struct AsyncReadHandle final : TypedHandle<AsyncReadHandle, uint32_t>
    {
    };


    struct ReadGroup final
    {
        Path m_sourcePath;
        AsyncIOOperation* m_operation = nullptr;
        AsyncReadHandle m_handle;
        void* m_stagingMemory = nullptr;
        size_t m_stagingMemorySize = 0;
        std::byte* m_destination = nullptr;
        size_t m_destinationSize = 0;
        size_t m_readSize = 0;
        Compression::Method m_compressionMethod = Compression::Method::kNone;
    };


    struct AsyncIOPhysicalRead final
    {
        Rc<AsyncIOCachedFile> m_file;
        size_t m_offset = 0;
        size_t m_size = 0;
        void* m_destination = nullptr;
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

        virtual festd::expected<Platform::FileHandle, ResultCode> OpenFile(festd::string_view filePath) = 0;

        virtual AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) = 0;
        virtual bool PollRequestCompletion(AsyncIOCompletion& completion) = 0;
        virtual void Cancel([[maybe_unused]] AsyncReadHandle handle) {}
    };


    struct AsyncIOOperation final
    {
        Priority m_priority = Priority::kNormal;
        std::atomic<uint32_t> m_pendingWork = 0;
        Rc<AsyncIOController> m_controller;
        AsyncReadCommandList m_commandList;
        Threading::SpinLock m_completionLock;
        InternalAsyncReadCommands::AsyncInvokeFunctorCommand m_completionCallback;
    };


    struct AsyncIOController final : public IAsyncController
    {
        FE_RTTI("4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

        AsyncIOController(Memory::SpinLockedPool<AsyncIOController>& pool)
            : m_pool(pool)
        {
        }

        ~AsyncIOController() override = default;

        void DoRelease() override;

        void Cancel() override;
        AsyncOperationStatus GetStatus() const override;
        ResultCode GetLastOperationResult() const override;

        Memory::SpinLockedPool<AsyncIOController>& m_pool;
        std::atomic<bool> m_cancellationRequested = false;
        std::atomic<AsyncOperationStatus> m_status = AsyncOperationStatus::kQueued;
        std::atomic<ResultCode> m_lastResult = ResultCode::kSuccess;
    };


    struct AsyncIOCachedFile final : public Memory::RefCountedObjectBase
    {
        AsyncIOCachedFile(Memory::SpinLockedPool<AsyncIOCachedFile>& pool)
            : m_pool(pool)
        {
        }

        [[nodiscard]] Platform::FileHandle GetFileHandle()
        {
            m_lastUseTime = Platform::GetTicks();
            return m_fileHandle;
        }

    private:
        friend AsyncIOOpenFileCache;

        void DoRelease() override;

        Memory::SpinLockedPool<AsyncIOCachedFile>& m_pool;
        uint64_t m_lastUseTime = 0;
        uint64_t m_nameHash = 0;
        Path m_path;
        Platform::FileHandle m_fileHandle;
    };


    struct AsyncIOOpenFileCache final
    {
        void Init(uint32_t cacheSize, IAsyncIOBackend* backend);
        void Shutdown();

        [[nodiscard]] festd::expected<Rc<AsyncIOCachedFile>, ResultCode> CreateFile(festd::string_view path);

        void CollectGarbage();

    private:
        void DeleteEntry(uint32_t entryIndex);

        IAsyncIOBackend* m_backend = nullptr;
        uint32_t m_cacheSize = 0;
        festd::vector<Rc<AsyncIOCachedFile>> m_entries;
        Memory::SpinLockedPool<AsyncIOCachedFile> m_entryPool{ "IO/Async/OpenFileCacheEntryPool" };
    };


    struct AsyncStreamIO final : public IAsyncStreamIO
    {
        FE_RTTI("1ADBD843-E841-4B14-96EA-4AA08C901084");

        AsyncStreamIO(Logger* logger, IJobSystem* jobSystem);
        ~AsyncStreamIO() override;

        Rc<IAsyncController> ExecuteCommandList(const AsyncReadCommandList& commandList, Priority priority) override;

    private:
        Threading::Thread m_thread;
        Threading::Event m_queueEvent;
        Logger* m_logger = nullptr;
        std::atomic<bool> m_exitRequested = false;

        IJobSystem* m_jobSystem = nullptr;
        Rc<IAsyncIOBackend> m_backend;

        AsyncIOOpenFileCache m_fileCache;

        TracyLockable(Threading::SpinLock, m_queueLock);
        festd::vector<AsyncIOOperation*> m_queue;
        festd::vector<AsyncIOOperation*> m_runningOperations;

        Memory::SpinLockedPool<AsyncIOOperation> m_operationPool{ "IO/Async/OperationPool" };
        Memory::SpinLockedPool<ReadGroup> m_groupPool{ "IO/Async/ReadGroupPool" };
        Memory::SpinLockedPool<AsyncIOController> m_controllerPool{ "IO/Async/ControllerPool" };

        void* m_stagingMemory = nullptr;
        Memory::TLSFAllocator m_stagingAllocator;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        void EnqueueImpl(AsyncIOOperation* operation);
        AsyncIOOperation* TryDequeue();
        void ProcessCommandList(AsyncIOOperation* operation);
        void ProcessBackendCompletions();
        bool TryFinalizeOperation(AsyncIOOperation* operation);
        void SchedulerThread();
    };
} // namespace FE::IO
