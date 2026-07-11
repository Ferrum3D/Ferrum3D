#pragma once
#include <Core/Base/StackTrace.h>
#include <Core/Compression/Compression.h>
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Memory/SegmentedBuffer.h>
#include <Core/Time/BaseTime.h>

namespace FE::IO
{
    struct ResolvedDataSource final
    {
        Path m_filePath;
        size_t m_byteOffset = 0;
        size_t m_byteSize = 0;

        [[nodiscard]] bool IsValid() const
        {
            return !m_filePath.empty();
        }
    };


    struct AsyncReadBatch final
    {
        explicit AsyncReadBatch(WaitGroup* completionWaitGroup = nullptr)
        {
            m_callStack = Trace::CallStack::Capture();

            if (completionWaitGroup)
                SetCompletionWaitGroup(completionWaitGroup);
        }

        explicit AsyncReadBatch(const ResolvedDataSource& resolvedDataSource, WaitGroup* completionWaitGroup = nullptr)
            : AsyncReadBatch(completionWaitGroup)
        {
            SetSource(resolvedDataSource);
        }

        void SetSource(const ResolvedDataSource& resolvedDataSource);
        void SetCompletionWaitGroup(WaitGroup* waitGroup);

        void Read(void* destination, size_t destinationSize, size_t sourceOffset = 0);
        void Read(void* destination, size_t destinationSize, size_t compressedSize, Compression::Method compressionMethod,
                  size_t sourceOffset = 0);

        void Read(festd::span<std::byte> destination, size_t sourceOffset = 0);
        void Read(festd::span<std::byte> destination, size_t compressedSize, Compression::Method compressionMethod,
                  size_t sourceOffset = 0);

        // The data read from the file will be appended to the destination.
        // If bytesToRead is greater than sourceOffset + size of source file in bytes, the source file will be read until EOF.
        void ReadAppend(festd::pmr::vector<std::byte>& destination, size_t bytesToRead, size_t sourceOffset = 0);
        void ReadAppend(festd::pmr::vector<std::byte>& destination, Compression::Method compressionMethod, size_t compressedSize,
                        size_t uncompressedSize, size_t sourceOffset = 0);

        template<class TFunctor>
            requires(sizeof(TFunctor) <= 48)
        void InvokeOnCompletion(TFunctor&& functor)
        {
            if constexpr (std::is_invocable_v<TFunctor, IAsyncController*>)
            {
                m_completionCallback = std::forward<TFunctor>(functor);
            }
            else
            {
                m_completionCallback = [functor = std::forward<TFunctor>(functor)](IAsyncController*) {
                    functor();
                };
            }
        }

        struct Command final
        {
            union
            {
                std::byte* m_byteBuffer;
                festd::pmr::vector<std::byte>* m_vector;
            } m_destination = {};

            size_t m_sourceOffset = 0;
            size_t m_compressedSize = 0;
            size_t m_uncompressedSize = 0;
            Compression::Method m_compressionMethod = Compression::Method::kNone;
            bool m_vectorDestination = false;
        };

        Trace::CallStack m_callStack;
        ResolvedDataSource m_resolvedDataSource;
        festd::inline_vector<Command, 1> m_commands;
        festd::fixed_function<48, void(IAsyncController* controller)> m_completionCallback;
        Rc<WaitGroup> m_completionWaitGroup;
    };


    enum class AsyncOperationStatus
    {
        kQueued,
        kRunning,
        kCanceled,
        kSucceeded,
        kFailed,
    };


    constexpr bool IsFinalStatus(const AsyncOperationStatus status)
    {
        switch (status)
        {
        case AsyncOperationStatus::kCanceled:
        case AsyncOperationStatus::kSucceeded:
        case AsyncOperationStatus::kFailed:
            return true;
        default:
            return false;
        }
    }


    struct IAsyncController : public Memory::RefCountedObjectBase
    {
        FE_RTTI("2427B1D9-F1A5-4A1B-A804-EB9ACA502C28");

        ~IAsyncController() override = default;

        virtual void Cancel() = 0;
        virtual AsyncOperationStatus GetStatus() const = 0;
        virtual ResultCode GetLastOperationResult() const = 0;
    };


    struct IAsyncStreamIO : public Memory::RefCountedObjectBase
    {
        FE_RTTI("A44064EC-34E0-4B99-9BC7-A2B27321F617");

        ~IAsyncStreamIO() override = default;

        virtual Rc<IAsyncController> ReadBatch(AsyncReadBatch&& batch, Priority priority = Priority::kNormal) = 0;
        virtual Rc<IAsyncController> ReadBatch(const AsyncReadBatch& batch, Priority priority = Priority::kNormal) = 0;
    };
} // namespace FE::IO
