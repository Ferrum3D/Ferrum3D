#pragma once
#include <FeCore/Containers/ConcurrentQueue.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/SegmentedBuffer.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    namespace InternalAsyncCopyCommands
    {
        enum class AsyncCopyCommandType : uint32_t
        {
            kInvalid,
            kCopyBuffer,
            kCopyBufferContinuation,
            kUploadBuffer,
            kUploadTexture,
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
        void Free()
        {
            m_buffer.Free();
        }

        Memory::SegmentedBuffer m_buffer;
        WaitGroup* m_signalWaitGroup;
    };


    struct AsyncCopyCommandListBuilder final
    {
        explicit AsyncCopyCommandListBuilder(std::pmr::memory_resource* allocator, const uint32_t pageSize)
            : m_bufferBuilder(allocator, pageSize)
        {
        }

        void CopyBuffer(const Buffer* source, const Buffer* destination);
        void CopyBuffer(const Buffer* source, const Buffer* destination, uint32_t sourceOffset, uint32_t destinationOffset,
                        uint32_t size);

        void UploadBuffer(const Buffer* buffer, const void* data);
        void UploadBuffer(const Buffer* buffer, const void* data, uint32_t sourceOffset, uint32_t destinationOffset,
                          uint32_t size);

        void UploadTexture(const Texture* texture, const void* data, uint32_t sourceOffset, ImageSubresource subresource);

        AsyncCopyCommandList Build(WaitGroup* signalWaitGroup = nullptr);

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


    inline void AsyncCopyCommandListBuilder::CopyBuffer(const Buffer* source, const Buffer* destination)
    {
        const BufferDesc& sourceDesc = source->GetDesc();
        const BufferDesc& destinationDesc = destination->GetDesc();
        FE_Assert(sourceDesc.m_size == destinationDesc.m_size);
        CopyBuffer(source, destination, 0, 0, sourceDesc.m_size);
    }


    inline void AsyncCopyCommandListBuilder::CopyBuffer(const Buffer* source, const Buffer* destination,
                                                        const uint32_t sourceOffset, const uint32_t destinationOffset,
                                                        const uint32_t size)
    {
        using namespace InternalAsyncCopyCommands;

        if (m_prev.m_type == AsyncCopyCommandType::kCopyBuffer && m_prev.m_copyBuffer.m_source == source
            && m_prev.m_copyBuffer.m_destination == destination)
        {
            AsyncCopyBufferContinuationCommand command;
            command.m_type = AsyncCopyCommandType::kCopyBufferContinuation;
            command.m_sourceOffset = sourceOffset;
            command.m_destinationOffset = destinationOffset;
            command.m_size = size;
            m_bufferBuilder.WriteBytes(&command, sizeof(command));
        }
        else
        {
            AsyncCopyBufferCommand command;
            command.m_type = AsyncCopyCommandType::kCopyBuffer;
            command.m_source = source;
            command.m_destination = destination;
            command.m_sourceOffset = sourceOffset;
            command.m_destinationOffset = destinationOffset;
            command.m_size = size;
            m_bufferBuilder.WriteBytes(&command, sizeof(command));
        }

        m_prev.m_type = AsyncCopyCommandType::kCopyBuffer;
        m_prev.m_copyBuffer.m_source = source;
        m_prev.m_copyBuffer.m_destination = destination;
    }


    inline void AsyncCopyCommandListBuilder::UploadBuffer(const Buffer* buffer, const void* data)
    {
        UploadBuffer(buffer, data, 0, 0, buffer->GetDesc().m_size);
    }


    inline void AsyncCopyCommandListBuilder::UploadBuffer(const Buffer* buffer, const void* data, const uint32_t sourceOffset,
                                                          const uint32_t destinationOffset, const uint32_t size)
    {
        using namespace InternalAsyncCopyCommands;

        AsyncUploadBufferCommand command;
        command.m_type = AsyncCopyCommandType::kUploadBuffer;
        command.m_buffer = buffer;
        command.m_data = data;
        command.m_sourceOffset = sourceOffset;
        command.m_destinationOffset = destinationOffset;
        command.m_size = size;
        m_bufferBuilder.WriteBytes(&command, sizeof(command));
        m_prev.m_type = AsyncCopyCommandType::kUploadBuffer;
    }


    inline void AsyncCopyCommandListBuilder::UploadTexture(const Texture* texture, const void* data, const uint32_t sourceOffset,
                                                           const ImageSubresource subresource)
    {
        using namespace InternalAsyncCopyCommands;

        AsyncUploadTextureCommand command;
        command.m_type = AsyncCopyCommandType::kUploadTexture;
        command.m_texture = texture;
        command.m_subresource = subresource;
        command.m_data = data;
        command.m_sourceOffset = sourceOffset;
        m_bufferBuilder.WriteBytes(&command, sizeof(command));
        m_prev.m_type = AsyncCopyCommandType::kUploadTexture;
    }


    inline AsyncCopyCommandList AsyncCopyCommandListBuilder::Build(WaitGroup* signalWaitGroup)
    {
        AsyncCopyCommandList commandList;
        commandList.m_next = nullptr;
        commandList.m_buffer = m_bufferBuilder.Build();
        commandList.m_signalWaitGroup = signalWaitGroup;
        return commandList;
    }


    struct AsyncCopyQueue : public DeviceObject
    {
        FE_RTTI_Class(AsyncCopyQueue, "2C1855F0-034B-47B7-869A-F9512903212F");

        virtual void ExecuteCommandList(AsyncCopyCommandList* commandList) = 0;
        virtual void Drain() = 0;
    };
} // namespace FE::Graphics::Core
