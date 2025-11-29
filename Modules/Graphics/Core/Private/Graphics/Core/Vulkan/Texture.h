#pragma once
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool;

    struct Texture final : public Core::Texture
    {
        FE_RTTI("691EA96F-E1F3-47C5-BF5B-24258DFA57A8");

        ~Texture() override;

        static Texture* Create(Core::Device* device, Env::Name name, const Core::TextureDesc& desc);

        void CommitInternal(ResourcePool* resourcePool, Core::TextureCommitParams params);
        void SwapInternal(TextureInstance*& instance);

        void DecommitMemory() override;

        Core::ResourceMemory GetMemoryStatus() const override;

        [[nodiscard]] VkImageView GetSubresourceView(Core::TextureSubresource subresource) const;
        void SetState(Core::TextureSubresource subresource, Common::SubresourceState state);
        Common::SubresourceState GetState(Core::TextureSubresource subresource) const;

        void AddQueueReleaseBarrier(Core::TextureSubresource subresource, Common::SubresourceState state,
                                    Core::DeviceQueueType receiverQueue);
        festd::pmr::vector<Core::TextureSubresource> RetrieveQueueReleaseBarriers(Core::DeviceQueueType receiverQueue,
                                                                                std::pmr::memory_resource* allocator);

        [[nodiscard]] VkImage GetNative() const
        {
            FE_AssertDebug(m_instance);
            return m_instance->m_image;
        }

    private:
        explicit Texture(Core::Device* device, Env::Name name, const Core::TextureDesc& desc);

        void InitWholeImageView();
        void UpdateDebugNames();

        void SetStateNoLock(Core::TextureSubresource subresource, Common::SubresourceState state) const;

        Threading::SpinLock m_lock;

        Core::TextureSubresource m_wholeImageSubresource = Core::TextureSubresource::kInvalid;
        TextureInstance* m_instance = nullptr;

        festd::inline_vector<Core::TextureSubresource, 1>
            m_queueReleaseBarriers[festd::to_underlying(Core::DeviceQueueType::kCount)];
    };

    FE_ENABLE_NATIVE_CAST(Texture);
} // namespace FE::Graphics::Vulkan
