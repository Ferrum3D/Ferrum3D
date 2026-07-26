#pragma once
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Common/ResourceInstance.h>

namespace FE::Graphics::Common
{
    struct Buffer final : public Core::Buffer
    {
        FE_RTTI("6E88784E-1918-41AC-9343-0E93CF07C3B4");

        Buffer(Core::Device* device, Env::Name name, Core::BufferDesc desc);
        ~Buffer() override;

        Core::ResourceMemory GetMemoryStatus() const override;

        void DecommitMemory() override;

        [[nodiscard]] void* Map() override;
        void Unmap() override;
        void FlushMappedRange(uint32_t offset, uint32_t byteSize) override;

        void SetState(SubresourceState state);
        SubresourceState GetState() const;

        void AddQueueReleaseBarrier(const Core::BufferBarrierDesc& barrier);
        festd::optional<Core::BufferBarrierDesc> RetrieveQueueReleaseBarrier(Core::DeviceQueueType receiverQueue);

        void SwapInstance(ResourceInstance*& instance);
        void AssignInstance(ResourceInstance* instance)
        {
            SwapInstance(instance);
            FE_Assert(instance == nullptr);
        }

        ResourceInstance* GetInstance() const
        {
            return m_instance;
        }

    private:
        mutable Threading::SpinLock m_lock;
        festd::optional<Core::BufferBarrierDesc> m_queueReleaseBarriers[festd::to_underlying(Core::DeviceQueueType::kCount)];

        ResourceInstance* m_instance = nullptr;

        void DestroyObject() override;
    };


    inline Buffer* ImplCast(Core::Buffer* buffer)
    {
        return Rtti::AssertCast<Buffer*>(buffer);
    }

    inline const Buffer* ImplCast(const Core::Buffer* buffer)
    {
        return Rtti::AssertCast<const Buffer*>(buffer);
    }
} // namespace FE::Graphics::Common
