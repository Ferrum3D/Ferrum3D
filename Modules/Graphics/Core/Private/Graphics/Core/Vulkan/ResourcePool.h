#pragma once
#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Texture.h>
#include <festd/bit_vector.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool final : public Core::ResourcePool
    {
        ResourcePool(Core::Device* device);
        ~ResourcePool() override;

        FE_RTTI("32B0D24A-62EB-47D5-869D-897424FD3439");

        Core::Texture* CreateTexture(Env::Name name, Core::TextureDesc desc) override;
        Core::Buffer* CreateBuffer(Env::Name name, Core::BufferDesc desc) override;

        void CommitBufferMemory(Core::Buffer* buffer, const Core::ResourceCommitParams& params) override;
        void CommitTextureMemory(Core::Texture* texture, const Core::ResourceCommitParams& params) override;

        void DecommitBufferMemory(Core::Buffer* buffer) override;
        void DecommitTextureMemory(Core::Texture* texture) override;

        void EndFrame() override;

        [[nodiscard]] VmaAllocator GetAllocator() const
        {
            return m_vmaAllocator;
        }

    private:
        Threading::SpinLock m_lock;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

        uint32_t AllocateResourceSlot();

        template<class TDesc, class TParams>
        uint32_t FindFreeResource(const TDesc& desc, const TParams& params);

        festd::vector<ResourceInstance*> m_resources;
        festd::bit_vector m_freedResources;
        festd::bit_vector m_pendingResources;
        festd::bit_vector m_emptyResources;
    };

    FE_ENABLE_IMPL_CAST(ResourcePool);
} // namespace FE::Graphics::Vulkan
