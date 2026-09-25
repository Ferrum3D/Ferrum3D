#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/Common/ResourceInstance.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/AsyncCopyQueue.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <festd/bit_vector.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool final : public Core::ResourcePool
    {
        FE_RTTI("32B0D24A-62EB-47D5-869D-897424FD3439");

        explicit ResourcePool(Core::Device* device, Core::GraphicsQueue* graphicsQueue, Core::AsyncCopyQueue* asyncCopyQueue);
        ~ResourcePool() override;

        void CommitBufferMemory(Core::Buffer* buffer, const Core::ResourceCommitParams& params) override;
        void CommitTextureMemory(Core::Texture* texture, const Core::ResourceCommitParams& params) override;

        void DecommitBufferMemory(Core::Buffer* buffer) override;
        void DecommitTextureMemory(Core::Texture* texture) override;

        void EndFrame() override;

    private:
        struct ResourceSlot final
        {
            Core::DeviceQueueType m_ownerQueue = Core::DeviceQueueType::kCount;
            uint64_t m_expirationFrame = 0;
            bool m_isReusable = false;
        };

        uint32_t AllocateResourceSlot();

        template<class TDesc, class TParams>
        uint32_t FindFreeResource(const TDesc& desc, const TParams& params);

        void FinalizeDecommit(const Common::DetachedResourceInstance& detached);
        bool IsRetirementComplete(const Common::ResourceInstance* resourceInstance, const ResourceSlot& slot,
                                  uint64_t completedGraphicsFence, uint64_t completedTransferFence) const;
        void DestroyResource(uint32_t resourceIndex);

        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }

        Threading::SpinLock m_lock;

        GraphicsQueue* m_graphicsQueue = nullptr;
        AsyncCopyQueue* m_asyncCopyQueue = nullptr;

        festd::vector<Common::ResourceInstance*> m_resources;
        festd::vector<ResourceSlot> m_resourceSlots;
        festd::bit_vector m_freedResources;
        festd::bit_vector m_pendingResources;
        festd::bit_vector m_emptyResources;
        uint64_t m_frameIndex = 0;
    };

    FE_ENABLE_IMPL_CAST(ResourcePool);
} // namespace FE::Graphics::Vulkan
