#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Texture.h>
#include <festd/vector.h>

namespace FE::Graphics::Common
{
    union SubresourceState final
    {
        struct
        {
            Core::BarrierAccessFlags m_access : 24;
            Core::BarrierLayout m_layout : 8;
            Core::BarrierSyncFlags m_sync : 24;
            Core::DeviceQueueType m_queueType : 8;
        };

        uint64_t m_value = 0;
    };
    static_assert(sizeof(SubresourceState) == sizeof(uint64_t));


    struct ResourceInstance
    {
        FE_RTTI("B498017D-C07F-4022-8C3D-6F4C9CCF132B");

        ResourceInstance()
            : m_bindFlags(Core::BarrierAccessFlags::kNone)
            , m_memoryStatus(Core::ResourceMemory::kNotCommitted)
            , m_type(Core::ResourceType::kUnknown)
        {
        }

        virtual ~ResourceInstance() = default;
        ResourceInstance(const ResourceInstance&) = delete;
        ResourceInstance(ResourceInstance&&) = delete;
        ResourceInstance& operator=(const ResourceInstance&) = delete;
        ResourceInstance& operator=(ResourceInstance&&) = delete;

        Core::ResourcePool* m_pool = nullptr;
        Core::BarrierAccessFlags m_bindFlags : 26;
        Core::ResourceMemory m_memoryStatus : 4;
        Core::ResourceType m_type : 2;

        union
        {
            Core::BufferDesc m_bufferDesc;
            Core::TextureDesc m_textureDesc = {};
        };

        festd::inline_vector<SubresourceState, 1> m_subresourceStates;

        [[nodiscard]] uint32_t ScoreCompatibility(const Core::BufferDesc desc, const Core::BufferCommitParams& params) const
        {
            if (m_type != Core::ResourceType::kBuffer)
                return 0;

            if (m_memoryStatus != params.m_memory)
                return 0;

            if (m_bufferDesc != desc)
                return 0;

            if (!Bit::AllSet(m_bindFlags, params.m_bindFlags))
                return 0;

            const Core::BarrierAccessFlags unusedBindFlags = m_bindFlags & ~params.m_bindFlags;
            return 8 * sizeof(Core::BarrierAccessFlags) - Bit::PopCount(festd::to_underlying(unusedBindFlags));
        }

        [[nodiscard]] uint32_t ScoreCompatibility(const Core::TextureDesc desc, const Core::TextureCommitParams& params) const
        {
            if (m_type != Core::ResourceType::kTexture)
                return 0;

            if (m_memoryStatus != params.m_memory)
                return 0;

            if (m_textureDesc != desc)
                return 0;

            if (!Bit::AllSet(m_bindFlags, params.m_bindFlags))
                return 0;

            const Core::BarrierAccessFlags unusedBindFlags = m_bindFlags & ~params.m_bindFlags;
            if (Bit::AllSet(unusedBindFlags, Core::BarrierAccessFlags::kShaderWrite)
                && Bit::AllSet(params.m_bindFlags, Core::BarrierAccessFlags::kRenderTarget))
            {
                // Using render target as a UAV can disable some optimizations.
                return 0;
            }

            return 8 * sizeof(Core::BarrierAccessFlags) - Bit::PopCount(festd::to_underlying(unusedBindFlags));
        }
    };
} // namespace FE::Graphics::Common
