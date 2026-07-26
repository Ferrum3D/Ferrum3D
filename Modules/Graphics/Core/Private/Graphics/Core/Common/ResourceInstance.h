#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Texture.h>
#include <festd/vector.h>

namespace FE::Graphics::Common
{
    struct SubresourceState final
    {
        Core::BarrierAccessFlags m_access : 24 = Core::BarrierAccessFlags::kNone;
        Core::BarrierLayout m_layout : 8 = Core::BarrierLayout::kUndefined;
        Core::BarrierSyncFlags m_sync : 24 = Core::BarrierSyncFlags::kNone;
        Core::DeviceQueueType m_queueType : 8 = Core::DeviceQueueType::kGraphics;
    };
    static_assert(sizeof(SubresourceState) == sizeof(uint64_t));


    inline bool operator==(const SubresourceState lhs, const SubresourceState rhs)
    {
        return std::bit_cast<uint64_t>(lhs) == std::bit_cast<uint64_t>(rhs);
    }


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

        virtual void UpdateDebugNames(Core::Device* device, Env::Name name) = 0;

        virtual void* Map(Core::Device* device) = 0;
        virtual void Unmap(Core::Device* device) = 0;
        virtual void FlushMappedRange(Core::Device* device, uint32_t offset, uint32_t byteSize) = 0;

        festd::array<uint64_t, festd::to_underlying(Core::DeviceQueueType::kCount)> m_lastFenceValues = {};

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

        Env::Name m_name;
        Core::TextureSubresource m_wholeImageSubresource = {};

        [[nodiscard]] uint32_t ScoreCompatibility(const Core::BufferDesc desc, const Core::ResourceCommitParams& params) const
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

        [[nodiscard]] uint32_t ScoreCompatibility(const Core::TextureDesc desc, const Core::ResourceCommitParams& params) const
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
