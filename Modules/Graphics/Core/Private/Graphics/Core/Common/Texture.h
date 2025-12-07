#pragma once
#include <Graphics/Core/Common/ResourceInstance.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Common
{
    struct Texture : public Core::Texture
    {
        Core::ResourceMemory GetMemoryStatus() const override;

        void SetState(Core::TextureSubresource subresource, SubresourceState state);
        SubresourceState GetState(Core::TextureSubresource subresource) const;

        void AddQueueReleaseBarrier(const Core::TextureBarrierDesc& barrier);
        festd::optional<Core::TextureBarrierDesc> RetrieveQueueReleaseBarrier(Core::DeviceQueueType receiverQueue,
                                                                              Core::TextureSubresource subresource);

        void SetQueueOwnership(Core::TextureSubresource subresource, Core::DeviceQueueType queue);

        const ResourceInstance* GetInstance() const
        {
            return m_instance;
        }

    protected:
        Threading::SpinLock m_lock;
        Core::TextureSubresource m_wholeImageSubresource = Core::TextureSubresource::kInvalid;
        festd::inline_vector<Core::TextureBarrierDesc, 1>
            m_queueReleaseBarriers[festd::to_underlying(Core::DeviceQueueType::kCount)];

        ResourceInstance* m_instance = nullptr;
    };
} // namespace FE::Graphics::Common
