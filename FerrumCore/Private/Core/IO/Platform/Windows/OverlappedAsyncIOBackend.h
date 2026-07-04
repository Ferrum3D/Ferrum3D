#pragma once
#include <Core/Base/PlatformInclude.h>
#include <Core/Containers/ConcurrentQueue.h>
#include <Core/Containers/SegmentedVector.h>
#include <Core/IO/AsyncStreamIO.h>
#include <festd/bit_vector.h>
#include <festd/ring_buffer.h>

#if !FE_PLATFORM_WINDOWS
#    error "OverlappedAsyncIOBackend is Windows-only"
#endif

namespace FE::IO
{
    struct OverlappedAsyncIOBackend final : public IAsyncIOBackend
    {
        FE_RTTI("70064B01-C464-4AD7-8169-C61C9D37419A");

        OverlappedAsyncIOBackend(Threading::Event& completionEvent);
        ~OverlappedAsyncIOBackend() override;

        festd::expected<Platform::FileHandle, ResultCode> OpenFile(festd::string_view filePath) override;
        AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) override;
        bool PollRequestCompletion(AsyncIOCompletion& completion) override;
        void Cancel(AsyncReadHandle handle) override;

    private:
        static constexpr uint32_t kMaxOutstandingReads = 32;

        enum class IocpThreadCommand : uint32_t
        {
            kExit,
        };

        struct NativeReadRequest final : public ConcurrentOnceConsumedQueue::Node
        {
            OVERLAPPED m_overlapped = {};
            HANDLE m_file = INVALID_HANDLE_VALUE;
            void* m_destination = nullptr;
            size_t m_size = 0;

            AsyncIOCompletion m_completion = {};

            void Invalidate()
            {
                *this = {};
            }
        };

        void StartRead(NativeReadRequest& read);
        void DispatchQueuedReads();

        HANDLE m_completionPort = nullptr;
        Threading::Thread m_iocpWaitThread;
        Threading::Event m_iocpThreadQuitEvent;
        Threading::Event& m_completionEvent;

        uint32_t m_dispatchedReadCount = 0;

        ConcurrentOnceConsumedQueue m_finishedReadsQueue;
        festd::vector<NativeReadRequest*> m_finishedReads;

        SegmentedVector<NativeReadRequest> m_readRequests;
        festd::bit_vector m_freeRequests;
        festd::ring_buffer<uint32_t> m_pendingRequests;
    };
} // namespace FE::IO
