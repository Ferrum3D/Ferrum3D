#pragma once
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool;

    struct Texture final : public Common::Texture
    {
        FE_RTTI("691EA96F-E1F3-47C5-BF5B-24258DFA57A8");

        ~Texture() override;

        static Texture* Create(Core::Device* device, Env::Name name, const Core::TextureDesc& desc);

        void DecommitMemory() override;
        void CommitInternal(ResourcePool* resourcePool, Core::TextureCommitParams params);
        void SwapInternal(TextureInstance*& instance);

        [[nodiscard]] VkImageView GetSubresourceView(Core::TextureSubresource subresource) const;

        [[nodiscard]] VkImage GetNative() const
        {
            FE_AssertDebug(m_instance);
            return RTTI::AssertCast<TextureInstance*>(m_instance)->m_image;
        }

    private:
        explicit Texture(Core::Device* device, Env::Name name, const Core::TextureDesc& desc);

        void InitWholeImageView();
        void UpdateDebugNames();
    };

    FE_ENABLE_NATIVE_CAST(Texture);
} // namespace FE::Graphics::Vulkan
