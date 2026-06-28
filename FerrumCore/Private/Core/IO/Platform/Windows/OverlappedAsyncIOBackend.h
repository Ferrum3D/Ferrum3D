#pragma once
#include <Core/Base/PlatformInclude.h>
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

        OverlappedAsyncIOBackend();
        ~OverlappedAsyncIOBackend() override;

        festd::expected<Platform::FileHandle, ResultCode> OpenFile(festd::string_view filePath) override;
        AsyncReadHandle DispatchRead(const AsyncIOPhysicalRead& read) override;
        bool PollRequestCompletion(AsyncIOCompletion& completion) override;
        void Cancel(AsyncReadHandle handle) override;

    private:
        static constexpr uint32_t kMaxOutstandingReads = 32;

        struct PendingRead final
        {
            OVERLAPPED m_overlapped = {};
            HANDLE m_file = INVALID_HANDLE_VALUE;
            AsyncReadHandle m_handle;
            void* m_destination = nullptr;
            size_t m_size = 0;
            ReadGroup* m_group = nullptr;
            bool m_started = false;

            void Invalidate()
            {
                *this = {};
            }
        };

        AsyncReadHandle StartRead(PendingRead* read);
        void CloseRead(PendingRead* read);
        PendingRead* FindRead(AsyncReadHandle handle);
        void DispatchQueuedReads();

        HANDLE m_completionPort = nullptr;
        Threading::Thread m_iocpWaitThread;

        Threading::Mutex m_queueLock;
        festd::fixed_ring_buffer<PendingRead, kMaxOutstandingReads> m_readQueue;

        festd::fixed_bit_vector<kMaxOutstandingReads> m_freeReads;
        festd::array<PendingRead, kMaxOutstandingReads> m_outstandingReads;
    };
} // namespace FE::IO
