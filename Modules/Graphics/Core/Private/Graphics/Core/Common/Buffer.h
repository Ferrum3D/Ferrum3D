#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Common/ResourceInstance.h>

namespace FE::Graphics::Common
{
    struct Buffer : public Core::Buffer
    {
        FE_RTTI("6E88784E-1918-41AC-9343-0E93CF07C3B4");

        Core::ResourceMemory GetMemoryStatus() const override;

        void SetState(SubresourceState state);
        SubresourceState GetState() const;

        void AddQueueReleaseBarrier(const Core::BufferBarrierDesc& barrier);
        festd::optional<Core::BufferBarrierDesc> RetrieveQueueReleaseBarrier(Core::DeviceQueueType receiverQueue);

        const ResourceInstance* GetInstance() const
        {
            return m_instance;
        }

    protected:
        mutable Threading::SpinLock m_lock;
        festd::optional<Core::BufferBarrierDesc> m_queueReleaseBarriers[festd::to_underlying(Core::DeviceQueueType::kCount)];

        ResourceInstance* m_instance = nullptr;
    };
} // namespace FE::Graphics::Common
