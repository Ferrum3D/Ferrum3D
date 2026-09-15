#pragma once
#include <Core/IO/Artifact.h>
#include <Core/IO/Async.h>
#include <Core/Threading/Mutex.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <atomic>
#include <festd/vector.h>

namespace FE::Graphics
{
    enum class AssetStreamingOperationType : uint8_t
    {
        kCreateResource,
        kStreamIn,
        kStreamOut,
    };


    enum class AssetStreamingOperationStatus : uint8_t
    {
        kPending,
        kSucceeded,
        kFailed,
    };


    struct AssetStreamingOperation
    {
        virtual ~AssetStreamingOperation() = default;

        [[nodiscard]] AssetStreamingOperationType GetType() const
        {
            return m_type;
        }

        [[nodiscard]] uint32_t GetTargetIndex() const
        {
            return m_targetIndex;
        }

        virtual AssetStreamingOperationStatus Tick() = 0;
        virtual void Cancel() = 0;
        virtual void Destroy() = 0;

    protected:
        struct PayloadRead final
        {
            uint32_t m_payloadIndex = 0;
            festd::pmr::vector<std::byte> m_data;
            Rc<IO::Async::IController> m_controller;
        };

        AssetStreamingOperation(AssetStreamingOperationType type, uint32_t targetIndex, Core::AsyncCopyQueue* asyncCopyQueue);

        bool ReadPayloads(const IO::ArtifactRecord& artifact, uint32_t firstPayload, uint32_t payloadCount);
        void CancelAndWait();
        [[nodiscard]] bool IsCancellationRequested() const;

        AssetStreamingOperationType m_type;
        uint32_t m_targetIndex = 0;
        Core::AsyncCopyQueue* m_asyncCopyQueue = nullptr;
        festd::vector<PayloadRead> m_reads;
        Rc<WaitGroup> m_prepareDone;
        Rc<WaitGroup> m_uploadDone;
        Core::AsyncCopyCommandList* m_commandList = nullptr;
        std::atomic_bool m_cancelRequested = false;
        Threading::Mutex m_readMutex;
        bool m_prepareSucceeded = false;
        bool m_uploadSubmitted = false;
    };
} // namespace FE::Graphics
