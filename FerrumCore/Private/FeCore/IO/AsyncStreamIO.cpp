#include <FeCore/IO/AsyncStreamIO.h>
#include <FeCore/Logging/Trace.h>

namespace FE::IO
{
    struct AsyncReadController final : public IAsyncController
    {
        AsyncReadRequestQueueEntry* pRequestEntry = nullptr;

        FE_RTTI_Class(AsyncReadController, "4F28D2D7-1AB4-4279-A3BD-A1D15B2F5BA9");

        explicit AsyncReadController(AsyncReadRequestQueueEntry* pEntry)
            : pRequestEntry(pEntry)
        {
        }

        ~AsyncReadController() override = default;

        void Cancel() override
        {
            pRequestEntry->m_cancellationRequested.store(true, std::memory_order_release);
        }

        AsyncOperationStatus GetStatus() const override
        {
            return pRequestEntry->m_status.load(std::memory_order_acquire);
        }

        ResultCode GetLastOperationResult() const override
        {
            return pRequestEntry->m_lastResult.load(std::memory_order_acquire);
        }
    };


    AsyncReadRequestQueueEntry* AsyncStreamIO::TryDequeue()
    {
        ZoneScoped;

        std::lock_guard lk{ m_queueLock };
        if (m_queue.empty())
            return nullptr;

        AsyncReadRequestQueueEntry* pEntry = m_queue.front();
        m_queue.erase(m_queue.begin());
        return pEntry;
    }


    void AsyncStreamIO::ProcessRequest(AsyncReadRequestQueueEntry* entry)
    {
        ZoneScoped;

        constexpr uint32_t kSuccessColor = 0x4b4e6d;
        constexpr uint32_t kFailureColor = 0x9a031e;

        AsyncOperationStatus status = AsyncOperationStatus::kSucceeded;
        if (entry->m_status.load(std::memory_order_relaxed) == AsyncOperationStatus::kCanceled)
            status = AsyncOperationStatus::kCanceled;

        AsyncReadRequest& request = entry->m_request;
        if (request.pStream == nullptr && status != AsyncOperationStatus::kCanceled)
        {
            if (const auto result = m_streamFactory->OpenFileStream(request.Path, OpenMode::kReadOnly))
            {
                request.pStream = result.value();
                const StringSlice zoneText = request.pStream->GetName();
                ZoneText(zoneText.Data(), zoneText.Size());
            }
            else
            {
                const StringSlice resultDesc = GetResultDesc(result.error());
                status = AsyncOperationStatus::kFailed;
                ZoneTextF("Failed request: %.*s", resultDesc.Size(), resultDesc.Data());
                ZoneColor(kFailureColor);
            }
        }

        if (request.pAllocator == nullptr)
            request.pAllocator = std::pmr::get_default_resource();

        AsyncReadResult result{};
        result.pController = entry->m_pController.Get();
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

        entry->m_status.store(status, std::memory_order_release);
        request.pCallback->AsyncIOCallback(result);
    }


    void AsyncStreamIO::ReaderThread()
    {
        while (true)
        {
            if (m_exitRequested)
                break;

            AsyncReadRequestQueueEntry* pEntry = TryDequeue();
            if (pEntry == nullptr)
            {
                m_queueEvent.Wait();
                continue;
            }

            if (pEntry->m_cancellationRequested.load(std::memory_order_acquire))
            {
                pEntry->m_status.store(AsyncOperationStatus::kCanceled, std::memory_order_release);
            }
            else
            {
                pEntry->m_status.store(AsyncOperationStatus::kRunning, std::memory_order_release);
            }

            ProcessRequest(pEntry);
            Memory::Delete(&m_requestPool, pEntry, sizeof(*pEntry));
        }
    }


    AsyncStreamIO::AsyncStreamIO(Logger* pLogger, IStreamFactory* pStreamFactory)
        : m_logger(pLogger)
        , m_streamFactory(pStreamFactory)
        , m_requestPool("AsyncReadRequest", sizeof(AsyncReadRequestQueueEntry), 64 * 1024)
        , m_controllerPool("AsyncReadController", sizeof(AsyncReadController), 64 * 1024)
    {
        const auto threadFunc = [](uintptr_t userData) {
            reinterpret_cast<AsyncStreamIO*>(userData)->ReaderThread();
        };

        m_thread = CreateThread("Async IO Thread", threadFunc, reinterpret_cast<uintptr_t>(this));
        m_queueEvent = Threading::Event::CreateAutoReset();
    }


    AsyncStreamIO::~AsyncStreamIO()
    {
        m_exitRequested = true;
        m_queueEvent.Send();
        CloseThread(m_thread);
    }


    void AsyncStreamIO::ReadAsync(const AsyncReadRequest& request, IAsyncController** ppController)
    {
        std::lock_guard lk{ m_queueLock };

        auto* pEntry = Memory::New<AsyncReadRequestQueueEntry>(&m_requestPool);
        auto* pController = Rc<AsyncReadController>::New(&m_controllerPool, pEntry);
        pEntry->m_request = request;
        pEntry->m_pController = pController;

        const auto iter = eastl::upper_bound(
            m_queue.begin(), m_queue.end(), request.Priority, [](Priority lhs, AsyncReadRequestQueueEntry* rhs) {
                return lhs < rhs->m_request.Priority;
            });

        m_queue.insert(iter, pEntry);

        if (ppController)
            *ppController = pController;

        m_queueEvent.Send();
    }
} // namespace FE::IO
