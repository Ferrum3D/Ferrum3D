#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/AsyncCopyQueue.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Core/Vulkan/Texture.h>
#include <festd/bit_vector.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool final : public Core::ResourcePool
    {
        FE_RTTI("32B0D24A-62EB-47D5-869D-897424FD3439");

        explicit ResourcePool(Core::Device* device, Core::GraphicsQueue* graphicsQueue, Core::AsyncCopyQueue* asyncCopyQueue);
        ~ResourcePool() override;

        Core::Texture* CreateTexture(Env::Name name, Core::TextureDesc desc) override;
        Core::Buffer* CreateBuffer(Env::Name name, Core::BufferDesc desc) override;

        void CommitBufferMemory(Core::Buffer* buffer, const Core::ResourceCommitParams& params) override;
        void CommitTextureMemory(Core::Texture* texture, const Core::ResourceCommitParams& params) override;

        void DecommitBufferMemory(Core::Buffer* buffer) override;
        void DecommitTextureMemory(Core::Texture* texture) override;

        void EndFrame() override;

    private:
        uint32_t AllocateResourceSlot();

        template<class TDesc, class TParams>
        uint32_t FindFreeResource(const TDesc& desc, const TParams& params);

        void FinalizeDecommit(ResourceInstance* resourceInstance);

        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }

        Threading::SpinLock m_lock;

        GraphicsQueue* m_graphicsQueue = nullptr;
        AsyncCopyQueue* m_asyncCopyQueue = nullptr;

        festd::vector<ResourceInstance*> m_resources;
        festd::bit_vector m_freedResources;
        festd::bit_vector m_pendingResources;
        festd::bit_vector m_emptyResources;
    };

    FE_ENABLE_IMPL_CAST(ResourcePool);
} // namespace FE::Graphics::Vulkan
