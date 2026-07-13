#pragma once
#include <Core/IO/Async.h>
#include <Core/Logging/Logger.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Event.h>
#include <Core/Threading/Thread.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::IO::Async
{
    struct Operation;
    struct Controller;
    struct CachedFile;
    struct OpenFileCache;


    namespace Internal
    {
        void Init(std::pmr::memory_resource* allocator);
        void Shutdown();
    } // namespace Internal


    struct AsyncReadHandle final : TypedHandle<AsyncReadHandle, uint32_t>
    {
    };


    struct ReadGroup final
    {
        Path m_sourcePath;
        Operation* m_operation = nullptr;
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
        Rc<CachedFile> m_file;
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


    struct Operation final
    {
        Priority m_priority = Priority::kNormal;
        std::atomic<uint32_t> m_pendingWork = 0;
        Rc<Controller> m_controller;
        Batch m_batch;
        Threading::SpinLock m_completionLock;
    };


    struct Controller final : public IController
    {
        FE_RTTI("4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

        Controller(Memory::SpinLockedPool<Controller>& pool)
            : m_pool(pool)
        {
        }

        ~Controller() override = default;

        void DoRelease() override;

        void Cancel() override;
        Status GetStatus() const override;
        ResultCode GetLastOperationResult() const override;

        Memory::SpinLockedPool<Controller>& m_pool;
        std::atomic<bool> m_cancellationRequested = false;
        std::atomic<Status> m_status = Status::kQueued;
        std::atomic<ResultCode> m_lastResult = ResultCode::kSuccess;
    };


    struct CachedFile final : public Memory::RefCountedObjectBase
    {
        CachedFile(Memory::SpinLockedPool<CachedFile>& pool)
            : m_pool(pool)
        {
        }

        [[nodiscard]] Platform::FileHandle GetFileHandle()
        {
            m_lastUseTime = Platform::GetTicks();
            return m_fileHandle;
        }

    private:
        friend OpenFileCache;

        void DoRelease() override;

        Memory::SpinLockedPool<CachedFile>& m_pool;
        uint64_t m_lastUseTime = 0;
        uint64_t m_nameHash = 0;
        Path m_path;
        Platform::FileHandle m_fileHandle;
    };


    struct OpenFileCache final
    {
        void Init(uint32_t cacheSize, IAsyncIOBackend* backend);
        void Shutdown();

        [[nodiscard]] festd::expected<Rc<CachedFile>, ResultCode> CreateFile(festd::string_view path);

        void CollectGarbage();

    private:
        void DeleteEntry(uint32_t entryIndex);

        IAsyncIOBackend* m_backend = nullptr;
        uint32_t m_cacheSize = 0;
        festd::vector<Rc<CachedFile>> m_entries;
        Memory::SpinLockedPool<CachedFile> m_entryPool{ "IO/Async/OpenFileCacheEntryPool" };
    };


    struct SchedulerImpl final
    {
        SchedulerImpl();
        ~SchedulerImpl();

        Rc<IController> Read(const Batch& batch, Priority priority);
        Rc<IController> Read(Batch&& batch, Priority priority);

        static SchedulerImpl& Get();

    private:
        Threading::Thread m_thread;
        Threading::Event m_queueEvent;
        std::atomic<bool> m_exitRequested = false;

        Rc<IAsyncIOBackend> m_backend;
        OpenFileCache m_fileCache;

        TracyLockable(Threading::SpinLock, m_queueLock);
        festd::vector<Operation*> m_queue;
        festd::vector<Operation*> m_runningOperations;

        Memory::SpinLockedPool<Operation> m_operationPool{ "IO/Async/OperationPool" };
        Memory::SpinLockedPool<ReadGroup> m_groupPool{ "IO/Async/ReadGroupPool" };
        Memory::SpinLockedPool<Controller> m_controllerPool{ "IO/Async/ControllerPool" };

        void* m_stagingMemory = nullptr;
        Memory::TLSFAllocator m_stagingAllocator;

        void EnqueueImpl(Operation* operation);
        Operation* TryDequeue();
        void ProcessOperation(Operation* operation);
        void ProcessBackendCompletions();
        bool TryFinalizeOperation(const Operation* operation);
        void SchedulerThread();
    };
} // namespace FE::IO::Async
