#include <Graphics/Core/AsyncCopyQueue.h>

namespace FE::Graphics::Core
{
    void AsyncCopyCommandListBuilder::CopyBuffer(const Buffer* source, const Buffer* destination)
    {
        const BufferDesc& sourceDesc = source->GetDesc();
        const BufferDesc& destinationDesc = destination->GetDesc();
        FE_Assert(sourceDesc.m_size == destinationDesc.m_size);
        CopyBuffer(source, destination, 0, 0, sourceDesc.m_size);
    }


    void AsyncCopyCommandListBuilder::CopyBuffer(const Buffer* source, const Buffer* destination, const uint32_t sourceOffset,
                                                 const uint32_t destinationOffset, const uint32_t size)
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


    void AsyncCopyCommandListBuilder::UploadBuffer(const Buffer* buffer, const void* data)
    {
        UploadBuffer(buffer, data, 0, 0, buffer->GetDesc().m_size);
    }


    void AsyncCopyCommandListBuilder::UploadBuffer(const Buffer* buffer, const void* data, const uint32_t sourceOffset,
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


    void AsyncCopyCommandListBuilder::UploadTexture(const Texture* texture, const void* data, const uint32_t sourceOffset,
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


    AsyncCopyCommandList AsyncCopyCommandListBuilder::Build(WaitGroup* signalWaitGroup)
    {
        AsyncCopyCommandList commandList;
        commandList.m_next = nullptr;
        commandList.m_buffer = m_bufferBuilder.Build();
        commandList.m_signalWaitGroup = signalWaitGroup;
        commandList.m_allocator = nullptr;
        return commandList;
    }
} // namespace FE::Graphics::Core
