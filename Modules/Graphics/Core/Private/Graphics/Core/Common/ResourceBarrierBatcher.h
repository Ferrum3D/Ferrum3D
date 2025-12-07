#pragma once
#include <Graphics/Core/Barrier.h>
#include <festd/vector.h>

namespace FE::Graphics::Common
{
    struct ResourceBarrierBatcher final
    {
        explicit ResourceBarrierBatcher(std::pmr::memory_resource* allocator);

        bool AddBarrier(const Core::TextureBarrierDesc& barrier);
        bool AddBarrier(const Core::BufferBarrierDesc& barrier);

        festd::pmr::inline_vector<Core::TextureBarrierDesc, 4> m_textureBarriers;
        festd::pmr::inline_vector<Core::BufferBarrierDesc, 4> m_bufferBarriers;
    };
} // namespace FE::Graphics::Common
