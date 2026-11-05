#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Fence.h>
#include <festd/vector.h>

namespace FE::Graphics::Common
{
    //! Linear allocator of per-frame shader constants in persistently mapped memory.
    //!
    //! Shaders read allocations through their device addresses, so they need no buffers, descriptors, copies or barriers of
    //! their own: submitting the frame makes host writes visible to the device. Chunks are reused once the frame that wrote
    //! them has completed.
    struct ConstantAllocator final
    {
        struct Allocation final
        {
            std::byte* m_hostPointer = nullptr;
            uint64_t m_deviceAddress = 0;
        };

        static constexpr uint32_t kAlignment = 16;

        explicit ConstantAllocator(Core::ResourcePool* resourcePool);
        ~ConstantAllocator();

        ConstantAllocator(const ConstantAllocator&) = delete;
        ConstantAllocator(ConstantAllocator&&) = delete;
        ConstantAllocator& operator=(const ConstantAllocator&) = delete;
        ConstantAllocator& operator=(ConstantAllocator&&) = delete;

        [[nodiscard]] Allocation Allocate(uint32_t byteSize);

        //! Whether the address belongs to an allocation of the frame being recorded.
        [[nodiscard]] bool IsCurrentFrameAddress(uint64_t deviceAddress) const;

        void BeginFrame();
        void EndFrame(const Core::FenceSyncPoint& fence);

    private:
        static constexpr uint32_t kChunkByteSize = 1024 * 1024;

        struct Chunk final
        {
            Rc<Core::Buffer> m_buffer;
            std::byte* m_hostPointer = nullptr;
            uint64_t m_deviceAddress = 0;
            uint32_t m_capacity = 0;
            uint32_t m_used = 0;
            Core::FenceSyncPoint m_fence;
        };

        [[nodiscard]] Chunk& AcquireChunk(uint32_t byteSize);
        [[nodiscard]] Chunk CreateChunk(uint32_t capacity) const;

        Core::ResourcePool* m_resourcePool = nullptr;
        festd::vector<Chunk> m_currentChunks;
        festd::vector<Chunk> m_pendingChunks;
        festd::vector<Chunk> m_freeChunks;
    };
} // namespace FE::Graphics::Common
