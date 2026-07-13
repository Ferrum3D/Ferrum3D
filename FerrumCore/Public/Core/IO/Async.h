#pragma once
#include <Core/Base/StackTrace.h>
#include <Core/Compression/Compression.h>
#include <Core/IO/BaseIO.h>
#include <Core/IO/Path.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Memory/SegmentedBuffer.h>
#include <Core/Time/BaseTime.h>

namespace FE::IO::Async
{
    struct Batch final
    {
        explicit Batch(WaitGroup* completionWaitGroup = nullptr)
        {
            m_callStack = Trace::CallStack::Capture();

            if (completionWaitGroup)
                SetCompletionWaitGroup(completionWaitGroup);
        }

        explicit Batch(const ResolvedDataSource& resolvedDataSource, WaitGroup* completionWaitGroup = nullptr)
            : Batch(completionWaitGroup)
        {
            SetSource(resolvedDataSource);
        }

        void SetSource(const ResolvedDataSource& resolvedDataSource);
        void SetSource(festd::string_view filePath, size_t fileSize = 0);
        void SetSource(const PathView& filePath, size_t fileSize = 0);

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

        void SetCompletionWaitGroup(WaitGroup* waitGroup);

        template<class TFunctor>
            requires(sizeof(TFunctor) <= 48)
        void InvokeOnCompletion(TFunctor&& functor)
        {
            FE_Assert(!m_completionCallback);

            if constexpr (std::is_invocable_v<TFunctor, IController*>)
            {
                m_completionCallback = std::forward<TFunctor>(functor);
            }
            else
            {
                m_completionCallback = [functor = std::forward<TFunctor>(functor)](IController*) {
                    functor();
                };
            }
        }

    private:
        friend struct SchedulerImpl;

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

        void ValidateRead(size_t offset, size_t byteSize) const;

        Trace::CallStack m_callStack;
        ResolvedDataSource m_resolvedDataSource;
        festd::inline_vector<Command, 1> m_commands;
        festd::fixed_function<48, void(IController* controller)> m_completionCallback;
        Rc<WaitGroup> m_completionWaitGroup;
    };


    struct IController : public Memory::RefCountedObjectBase
    {
        FE_RTTI("2427B1D9-F1A5-4A1B-A804-EB9ACA502C28");

        ~IController() override = default;

        virtual void Cancel() = 0;
        virtual Status GetStatus() const = 0;
        virtual ResultCode GetLastOperationResult() const = 0;
    };


    Rc<IController> Read(Batch&& batch, Priority priority = Priority::kNormal);
    Rc<IController> Read(const Batch& batch, Priority priority = Priority::kNormal);
} // namespace FE::IO::Async
