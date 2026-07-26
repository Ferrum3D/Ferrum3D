#pragma once
#include <Graphics/Core/Common/ResourceInstance.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Common
{
    struct Texture final : public Core::Texture
    {
        FE_RTTI("99F7A913-61EF-4C87-9B27-D06CA99F0D00");

        Texture(Core::Device* device, Env::Name name, const Core::TextureDesc& desc);
        ~Texture() override;

        void DecommitMemory() override;

        Core::ResourceMemory GetMemoryStatus() const override;

        void SetState(Core::TextureSubresource subresource, SubresourceState state);
        SubresourceState GetState(Core::TextureSubresource subresource) const;

        void AddQueueReleaseBarrier(const Core::TextureBarrierDesc& barrier);
        festd::optional<Core::TextureBarrierDesc> RetrieveQueueReleaseBarrier(Core::DeviceQueueType receiverQueue,
                                                                              Core::TextureSubresource subresource);

        void SetQueueOwnership(Core::TextureSubresource subresource, Core::DeviceQueueType queue);

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
        festd::inline_vector<Core::TextureBarrierDesc, 1>
            m_queueReleaseBarriers[festd::to_underlying(Core::DeviceQueueType::kCount)];

        ResourceInstance* m_instance = nullptr;

        void DestroyObject() override;
    };


    inline Texture* ImplCast(Core::Texture* texture)
    {
        return Rtti::AssertCast<Texture*>(texture);
    }

    inline const Texture* ImplCast(const Core::Texture* texture)
    {
        return Rtti::AssertCast<const Texture*>(texture);
    }
} // namespace FE::Graphics::Common
