#include <FeCore/IO/AsyncStreamIO.h>
#include <FeCore/Logging/Trace.h>

namespace FE::IO
{
    struct AsyncReadController final : public IAsyncController
    {
        AsyncReadRequestQueueEntry* pRequestEntry = nullptr;

        FE_RTTI_Class(AsyncReadController, "4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

        inline AsyncReadController(AsyncReadRequestQueueEntry* pEntry)
            : pRequestEntry(pEntry)
        {
        }

        ~AsyncReadController() override = default;

        inline void Cancel() override
        {
            pRequestEntry->CancellationRequested.store(true, std::memory_order_release);
        }

        inline AsyncOperationStatus GetStatus() const override
        {
            return pRequestEntry->Status.load(std::memory_order_acquire);
        }

        inline ResultCode GetLastOperationResult() const override
        {
            return pRequestEntry->LastResult.load(std::memory_order_acquire);
        }
    };


    AsyncReadRequestQueueEntry* AsyncStreamIO::TryDequeue()
    {
        ZoneScoped;

        std::lock_guard lk{ m_QueueLock };
        if (m_Queue.empty())
            return nullptr;

        AsyncReadRequestQueueEntry* pEntry = m_Queue.front();
        m_Queue.erase(m_Queue.begin());
        return pEntry;
    }


    void AsyncStreamIO::ProcessRequest(AsyncReadRequestQueueEntry* entry)
    {
        ZoneScoped;

        constexpr uint32_t kSuccessColor = 0x4b4e6d;
        constexpr uint32_t kFailureColor = 0x9a031e;

        AsyncOperationStatus status = AsyncOperationStatus::kSucceeded;
        if (entry->Status.load(std::memory_order_relaxed) == AsyncOperationStatus::kCanceled)
            status = AsyncOperationStatus::kCanceled;

        AsyncReadRequest& request = entry->Request;
        if (request.pStream == nullptr && status != AsyncOperationStatus::kCanceled)
        {
            const auto result = m_pStreamFactory->OpenFileStream(request.Path, OpenMode::kReadOnly);
            if (result)
            {
                request.pStream = result.Unwrap();
                const StringSlice zoneText = request.pStream->GetName();
                ZoneText(zoneText.Data(), zoneText.Size());
            }
            else
            {
                const StringSlice resultDesc = GetResultDesc(result.UnwrapErr());
                status = AsyncOperationStatus::kFailed;
                ZoneTextF("Failed request: %.*s", resultDesc.Size(), resultDesc.Data());
                ZoneColor(kFailureColor);
            }
        }

        if (request.pAllocator == nullptr)
            request.pAllocator = std::pmr::get_default_resource();

        AsyncReadResult result{};
        result.pController = entry->pController.Get();
        result.pRequest = &request;
        if (status != AsyncOperationStatus::kFailed && status != AsyncOperationStatus::kCanceled)
        {
            if (request.ReadBufferSize == 0)
                request.ReadBufferSize = static_cast<uint32_t>(request.pStream->Length() - request.Offset);

            if (request.pReadBuffer == nullptr)
            {
                request.pReadBuffer =
                    static_cast<std::byte*>(request.pAllocator->allocate(request.ReadBufferSize, Memory::kDefaultAlignment));
            }

            result.BytesRead = request.pStream->ReadToBuffer({ request.pReadBuffer, request.ReadBufferSize });
            status = AsyncOperationStatus::kSucceeded;
            ZoneColor(kSuccessColor);
        }

        entry->Status.store(status, std::memory_order_release);
        request.pCallback->AsyncIOCallback(result);
    }


    void AsyncStreamIO::ReaderThread()
    {
        while (true)
        {
            if (m_ExitRequested)
                break;

            AsyncReadRequestQueueEntry* pEntry = TryDequeue();
            if (pEntry == nullptr)
            {
                m_QueueEvent.Wait();
                continue;
            }

            if (pEntry->CancellationRequested.load(std::memory_order_acquire))
            {
                pEntry->Status.store(AsyncOperationStatus::kCanceled, std::memory_order_release);
            }
            else
            {
                pEntry->Status.store(AsyncOperationStatus::kRunning, std::memory_order_release);
            }

            ProcessRequest(pEntry);
            Memory::Delete(&m_RequestPool, pEntry, sizeof(*pEntry));
        }
    }


    AsyncStreamIO::AsyncStreamIO(Logger* pLogger, IStreamFactory* pStreamFactory)
        : m_pLogger(pLogger)
        , m_pStreamFactory(pStreamFactory)
        , m_RequestPool("AsyncReadRequest", sizeof(AsyncReadRequestQueueEntry), 64 * 1024)
        , m_ControllerPool("AsyncReadController", sizeof(AsyncReadController), 64 * 1024)
    {
        const auto threadFunc = [](uintptr_t userData) {
            reinterpret_cast<AsyncStreamIO*>(userData)->ReaderThread();
        };

        m_Thread = CreateThread("Async IO Thread", threadFunc, reinterpret_cast<uintptr_t>(this));
        m_QueueEvent = Threading::Event::CreateAutoReset();
    }


    AsyncStreamIO::~AsyncStreamIO()
    {
        m_ExitRequested = true;
        m_QueueEvent.Send();
        CloseThread(m_Thread);
    }


    void AsyncStreamIO::ReadAsync(const AsyncReadRequest& request, IAsyncController** ppController)
    {
        std::lock_guard lk{ m_QueueLock };

        auto* pEntry = Memory::New<AsyncReadRequestQueueEntry>(&m_RequestPool);
        auto* pController = Rc<AsyncReadController>::New(&m_ControllerPool, pEntry);
        pEntry->Request = request;
        pEntry->pController = pController;

        const auto iter = eastl::upper_bound(
            m_Queue.begin(), m_Queue.end(), request.Priority, [](Priority lhs, AsyncReadRequestQueueEntry* rhs) {
                return lhs < rhs->Request.Priority;
            });

        m_Queue.insert(iter, pEntry);

        if (ppController)
            *ppController = pController;

        m_QueueEvent.Send();
    }
} // namespace FE::IO
