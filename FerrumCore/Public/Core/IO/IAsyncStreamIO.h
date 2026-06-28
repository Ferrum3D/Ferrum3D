#pragma once
#include <Core/Compression/Compression.h>
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/Jobs/IJobSystem.h>
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
    };


    namespace InternalAsyncReadCommands
    {
        enum class AsyncReadCommandType : uint32_t
        {
            kInvalid,
            kSetSource,
            kInvokeFunctor,
            kRead,
            kReadCompressed,
        };


        struct AsyncSetSourceCommand final
        {
            AsyncReadCommandType m_type = AsyncReadCommandType::kSetSource;
            ResolvedDataSource m_source;
        };


        struct AsyncInvokeFunctorCommand final
        {
            AsyncReadCommandType m_type = AsyncReadCommandType::kInvokeFunctor;
            uint32_t m_functorSize = 0;
            void (*m_functor)(void* context) = nullptr;
            void* m_context = nullptr;
        };


        struct AsyncReadCommand final
        {
            AsyncReadCommandType m_type = AsyncReadCommandType::kRead;
            std::byte* m_destination = nullptr;
            size_t m_destinationSize = 0;
            size_t m_sourceOffset = 0;
        };


        struct AsyncReadCompressedCommand final
        {
            AsyncReadCommandType m_type = AsyncReadCommandType::kReadCompressed;
            std::byte* m_destination = nullptr;
            size_t m_destinationSize = 0;
            size_t m_sourceOffset = 0;
            size_t m_compressedSize = 0;
            Compression::Method m_compressionMethod = Compression::Method::kNone;
        };
    } // namespace InternalAsyncReadCommands


    struct AsyncReadCommandList final
    {
        Memory::SegmentedBuffer m_buffer;
        WaitGroup* m_signalWaitGroup = nullptr;
    };


    struct AsyncReadCommandListBuilder final
    {
        explicit AsyncReadCommandListBuilder(std::pmr::memory_resource* allocator, const uint32_t pageSize = 2048)
            : m_bufferBuilder(allocator ? allocator : std::pmr::get_default_resource(), pageSize)
        {
        }

        void* Allocate(size_t bytes, size_t alignment = Memory::kDefaultAlignment);
        void SetSource(const ResolvedDataSource& source);
        void Read(std::byte* destination, size_t destinationSize, size_t sourceOffset = 0);
        void Read(std::byte* destination, size_t destinationSize, size_t sourceOffset, size_t compressedSize,
                  Compression::Method compressionMethod);

        template<class TFunctor>
        void InvokeOnCompletion(TFunctor&& functor)
        {
            using namespace InternalAsyncReadCommands;

            const uint32_t functorSize = AlignUp<uint32_t>(sizeof(TFunctor), alignof(uintptr_t));

            FE_Assert(!m_completionCallbackSet);
            m_completionCallbackSet = true;

            AsyncInvokeFunctorCommand command;
            command.m_type = AsyncReadCommandType::kInvokeFunctor;
            command.m_functorSize = functorSize;
            command.m_functor = [](void* context) {
#if FE_DEVELOPMENT
                HighResolutionTimer timer;
                timer.Start();
#endif

                (*static_cast<TFunctor*>(context))();
                static_cast<TFunctor*>(context)->~TFunctor();

#if FE_DEVELOPMENT
                timer.Stop();

                if (const double ms = timer.GetElapsedMilliseconds(); ms > 1.0)
                {
                    const auto message =
                        Fmt::FixedFormat("AsyncReadCommandList completion callback took too long to execute ({} ms)", ms);
                    Trace::AssertionReport(SourceLocation::Current(), message.data(), message.size());
                }
#endif
            };

            void* commandPtr = m_bufferBuilder.WriteBytes(&command, sizeof(command));
            void* functorPtr = m_bufferBuilder.Allocate(functorSize);
            new (functorPtr) TFunctor(std::forward<TFunctor>(functor));

            static_cast<AsyncInvokeFunctorCommand*>(commandPtr)->m_context = functorPtr;
        }

        AsyncReadCommandList Build(WaitGroup* signalWaitGroup = nullptr);

    private:
        bool m_completionCallbackSet = false;
        Memory::SegmentedBufferBuilder m_bufferBuilder;
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

        virtual Rc<IAsyncController> ExecuteCommandList(const AsyncReadCommandList& commandList,
                                                        Priority priority = Priority::kNormal) = 0;
    };
} // namespace FE::IO
