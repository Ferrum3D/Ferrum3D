#pragma once
#include <Core/Memory/RefCount.h>
#include <Core/Memory/RingBufferAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Fence.h>
#include <festd/ring_buffer.h>

namespace FE::Graphics::Core
{
    struct RingUploader final
    {
        enum class Options : uint32_t
        {
            kNone = 0,
            kDisableBarriers = 1 << 0,
        };

        ~RingUploader();

        void Setup(Env::Name name, ResourcePool* resourcePool, uint32_t capacity);
        void Shutdown();

        [[nodiscard]] bool UploadBytes(FrameGraph& graph, BufferView destination, const void* source, uint32_t byteSize,
                                       Options options = Options::kNone);

        template<class T>
        [[nodiscard]] bool UploadArray(FrameGraph& graph, const BufferView destination, const festd::span<const T> source,
                                       const Options options = Options::kNone)
        {
            return UploadBytes(graph, destination, source.data(), source.size_bytes(), options);
        }

        template<class T>
        [[nodiscard]] bool UploadArray(FrameGraph& graph, const BufferView destination, const festd::span<T> source,
                                       const Options options = Options::kNone)
        {
            return UploadBytes(graph, destination, source.data(), source.size_bytes(), options);
        }

        template<class T>
            requires(std::is_trivially_copyable_v<T>)
        [[nodiscard]] bool Upload(FrameGraph& graph, const BufferView destination, const T& source,
                                  const Options options = Options::kNone)
        {
            return UploadBytes(graph, destination, &source, sizeof(T), options);
        }

        void CloseFrame(const FenceSyncPoint& fence);

    private:
        static constexpr uint32_t kAlignment = 256;

        struct FrameData final
        {
            FenceSyncPoint m_fence;
            uint32_t m_bytesAllocated = 0;
        };

        void CheckPendingFrames();

        uint32_t m_currentFrameBytes = 0;

        Rc<Buffer> m_buffer;
        std::byte* m_mappedMemory = nullptr;

        Memory::RingBufferAllocator m_ringBuffer;
        festd::ring_buffer<FrameData> m_pendingUploads;
    };

    FE_ENUM_OPERATORS(RingUploader::Options);
} // namespace FE::Graphics::Core
