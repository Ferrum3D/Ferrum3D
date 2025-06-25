#pragma once
#include <FeCore/Containers/ConcurrentQueue.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/SegmentedBuffer.h>
#include <FeCore/Time/BaseTime.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    namespace InternalAsyncCopyCommands
    {
        enum class AsyncCopyCommandType : uint32_t
        {
            kInvalid,
            kInvokeFunctor,
            kCopyBuffer,
            kCopyBufferContinuation,
            kUploadBuffer,
            kUploadTexture,
        };


        struct AsyncInvokeFunctorCommand final
        {
            AsyncCopyCommandType m_type;
            uint32_t m_functorSize;
            void (*m_functor)(void* context);
            void* m_context;
        };


        struct AsyncCopyBufferCommand final
        {
            AsyncCopyCommandType m_type;
            uint32_t m_sourceOffset;
            uint32_t m_destinationOffset;
            uint32_t m_size;
            const Buffer* m_source;
            const Buffer* m_destination;
        };


        struct AsyncCopyBufferContinuationCommand final
        {
            AsyncCopyCommandType m_type;
            uint32_t m_sourceOffset;
            uint32_t m_destinationOffset;
            uint32_t m_size;
        };


        struct AsyncUploadBufferCommand final
        {
            AsyncCopyCommandType m_type;
            uint32_t m_sourceOffset;
            uint32_t m_destinationOffset;
            uint32_t m_size;
            const Buffer* m_buffer;
            const void* m_data;
        };


        struct AsyncUploadTextureCommand final
        {
            AsyncCopyCommandType m_type;
            uint32_t m_sourceOffset;
            ImageSubresource m_subresource;
            const Texture* m_texture;
            const void* m_data;
        };
    } // namespace InternalAsyncCopyCommands


    struct AsyncCopyCommandList final : public ConcurrentOnceConsumedQueue::Node
    {
        Memory::SegmentedBuffer m_buffer;
        WaitGroup* m_signalWaitGroup;
        std::pmr::memory_resource* m_allocator;
    };


    struct AsyncCopyCommandListBuilder final
    {
        AsyncCopyCommandListBuilder(std::pmr::memory_resource* allocator, const uint32_t pageSize)
            : m_bufferBuilder(allocator, pageSize)
        {
        }

        template<class TFunctor>
        void Invoke(TFunctor&& functor)
        {
            using namespace InternalAsyncCopyCommands;

            const uint32_t functorSize = AlignUp<uint32_t>(sizeof(TFunctor), alignof(uintptr_t));

            AsyncInvokeFunctorCommand command;
            command.m_type = AsyncCopyCommandType::kInvokeFunctor;
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
                        Fmt::FixedFormat("AsyncCopyCommandListBuilder::Invoke functor took too long to execute ({} ms)", ms);
                    Trace::AssertionReport(SourceLocation::Current(), message.data(), message.size(), false);
                }
#endif
            };

            void* commandPtr = m_bufferBuilder.WriteBytes(&command, sizeof(command));
            void* functorPtr = m_bufferBuilder.Allocate(functorSize);
            new (functorPtr) TFunctor(std::forward<TFunctor>(functor));

            static_cast<AsyncInvokeFunctorCommand*>(commandPtr)->m_context = functorPtr;
            m_prev.m_type = AsyncCopyCommandType::kInvokeFunctor;
        }

        void CopyBuffer(const Buffer* source, const Buffer* destination);
        void CopyBuffer(const Buffer* source, const Buffer* destination, uint32_t sourceOffset, uint32_t destinationOffset,
                        uint32_t size);

        void UploadBuffer(const Buffer* buffer, const void* data);
        void UploadBuffer(const Buffer* buffer, const void* data, uint32_t sourceOffset, uint32_t destinationOffset,
                          uint32_t size);

        void UploadTexture(const Texture* texture, const void* data, uint32_t sourceOffset, ImageSubresource subresource);

        AsyncCopyCommandList Build(WaitGroup* signalWaitGroup = nullptr);

        AsyncCopyCommandList* Build(std::pmr::memory_resource* allocator, WaitGroup* signalWaitGroup = nullptr)
        {
            AsyncCopyCommandList commandList = Build(signalWaitGroup);
            commandList.m_allocator = allocator;
            return Memory::New<AsyncCopyCommandList>(allocator, commandList);
        }

    private:
        struct CommandState final
        {
            struct CopyBuffer final
            {
                const Buffer* m_source;
                const Buffer* m_destination;
            };

            InternalAsyncCopyCommands::AsyncCopyCommandType m_type = InternalAsyncCopyCommands::AsyncCopyCommandType::kInvalid;
            union
            {
                CopyBuffer m_copyBuffer;
            };
        };

        CommandState m_prev = {};
        Memory::SegmentedBufferBuilder m_bufferBuilder;
    };


    struct AsyncCopyQueue : public DeviceObject
    {
        FE_RTTI_Class(AsyncCopyQueue, "2C1855F0-034B-47B7-869A-F9512903212F");

        virtual void ExecuteCommandList(AsyncCopyCommandList* commandList) = 0;
        virtual void Drain() = 0;
    };
} // namespace FE::Graphics::Core
