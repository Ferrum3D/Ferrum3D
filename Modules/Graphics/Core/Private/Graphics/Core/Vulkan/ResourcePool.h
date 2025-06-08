#pragma once
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool final : public Core::ResourcePool
    {
        ResourcePool(Core::Device* device);
        ~ResourcePool() override;

        FE_RTTI_Class(ResourcePool, "32B0D24A-62EB-47D5-869D-897424FD3439");

        Core::Texture* CreateTexture(Env::Name name, const Core::ImageDesc& desc) override;
        Core::RenderTarget* CreateRenderTarget(Env::Name name, const Core::ImageDesc& desc) override;
        Core::Buffer* CreateBuffer(Env::Name name, const Core::BufferDesc& desc) override;

        [[nodiscard]] VmaAllocator GetAllocator() const
        {
            return m_vmaAllocator;
        }

    private:
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
    };

    FE_ENABLE_IMPL_CAST(ResourcePool);
} // namespace FE::Graphics::Vulkan
