#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    ResourcePool::ResourcePool(Core::Device* device, Core::GraphicsQueue* graphicsQueue, Core::AsyncCopyQueue* asyncCopyQueue)
    {
        m_device = device;
        m_graphicsQueue = ImplCast(graphicsQueue);
        m_asyncCopyQueue = ImplCast(asyncCopyQueue);

        SetImmediateDestroyPolicy();
    }


    ResourcePool::~ResourcePool()
    {
        m_device->WaitIdle();

        for (uint32_t resourceIndex = 0; resourceIndex < m_resources.size(); ++resourceIndex)
        {
            if (m_resources[resourceIndex])
                DestroyResource(resourceIndex);
        }

        if (Build::IsDebug())
        {
            festd::vector<const Core::Resource*> resources;
            festd::vector<const char*> names;
            for (const Core::Resource& resource : ImplCast(m_device)->GetResources())
            {
                resources.push_back(&resource);
                names.push_back(resource.GetName().c_str());
            }

            if (!resources.empty())
                FE_DebugBreak();
        }
    }


    template<class TDesc, class TParams>
    uint32_t ResourcePool::FindFreeResource(const TDesc& desc, const TParams& params)
    {
        const uint64_t completedGraphicsFence = m_graphicsQueue->GetCompletedFenceValue();
        const uint64_t completedTransferFence = m_asyncCopyQueue->GetCompletedFenceValue();
        uint32_t bestCompatibilityScore = 0;
        uint32_t bestResourceIndex = kInvalidIndex;
        Bit::Traverse(m_freedResources.view(), [&](const uint32_t resourceIndex) {
            const Common::ResourceInstance* resource = m_resources[resourceIndex];
            const ResourceSlot& slot = m_resourceSlots[resourceIndex];
            if (!slot.m_isReusable || slot.m_ownerQueue != params.m_queue)
                return;
            if (!IsRetirementComplete(resource, slot, completedGraphicsFence, completedTransferFence))
                return;

            const uint32_t compatibilityScore = resource->ScoreCompatibility(desc, params);
            if (compatibilityScore > bestCompatibilityScore)
            {
                bestCompatibilityScore = compatibilityScore;
                bestResourceIndex = resourceIndex;
            }
        });

        return bestResourceIndex;
    }


    void ResourcePool::CommitBufferMemory(Core::Buffer* buffer, const Core::ResourceCommitParams& params)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(buffer->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted);

        Common::ResourceInstance* instance;
        const uint32_t bestResourceIndex = FindFreeResource(buffer->GetDesc(), params);
        if (bestResourceIndex != kInvalidIndex)
        {
            instance = m_resources[bestResourceIndex];
            FE_Assert(instance);

            m_resources[bestResourceIndex] = nullptr;
            m_emptyResources.set(bestResourceIndex);
            m_freedResources.reset(bestResourceIndex);
        }
        else
        {
            auto* newInstance = BufferInstance::Create(buffer->GetDesc(), params, this);
            newInstance->Allocate(m_device);
            instance = newInstance;
        }

        instance->m_isTransient = params.m_isTransient;
        if (instance->m_subresourceStates.size() == 1)
            instance->m_subresourceStates.front().m_queueType = params.m_queue;

        Common::ImplCast(buffer)->AssignInstance(instance);
    }


    void ResourcePool::CommitTextureMemory(Core::Texture* texture, const Core::ResourceCommitParams& params)
    {
        std::unique_lock lk{ m_lock };

        FE_Assert(texture->GetMemoryStatus() == Core::ResourceMemory::kNotCommitted);

        Common::ResourceInstance* instance;
        const uint32_t bestResourceIndex = FindFreeResource(texture->GetDesc(), params);
        if (bestResourceIndex != kInvalidIndex)
        {
            instance = m_resources[bestResourceIndex];
            FE_Assert(instance);

            m_resources[bestResourceIndex] = nullptr;
            m_emptyResources.set(bestResourceIndex);
            m_freedResources.reset(bestResourceIndex);
        }
        else
        {
            auto* newInstance = TextureInstance::Create(texture->GetDesc(), params, this);
            newInstance->Allocate(m_device);
            instance = newInstance;
        }

        instance->m_isTransient = params.m_isTransient;
        if (instance->m_subresourceStates.size() == 1)
            instance->m_subresourceStates.front().m_queueType = params.m_queue;

        Common::ImplCast(texture)->AssignInstance(instance);
    }


    void ResourcePool::DecommitBufferMemory(Core::Buffer* buffer)
    {
        std::unique_lock lk{ m_lock };

        FinalizeDecommit(Common::ImplCast(buffer)->DetachInstance());
    }


    void ResourcePool::DecommitTextureMemory(Core::Texture* texture)
    {
        std::unique_lock lk{ m_lock };

        FinalizeDecommit(Common::ImplCast(texture)->DetachInstance());
    }


    void ResourcePool::EndFrame()
    {
        std::unique_lock lk{ m_lock };

        const uint64_t completedGraphicsFence = m_graphicsQueue->GetCompletedFenceValue();
        const uint64_t completedTransferFence = m_asyncCopyQueue->GetCompletedFenceValue();

        festd::vector<uint32_t> resourcesToDestroy;
        Bit::Traverse(m_freedResources.view(), [&](const uint32_t resourceIndex) {
            const Common::ResourceInstance* resource = m_resources[resourceIndex];
            const ResourceSlot& slot = m_resourceSlots[resourceIndex];
            FE_Assert(resource);

            if (IsRetirementComplete(resource, slot, completedGraphicsFence, completedTransferFence))
                resourcesToDestroy.push_back(resourceIndex);
        });

        for (const uint32_t resourceIndex : resourcesToDestroy)
            DestroyResource(resourceIndex);
        resourcesToDestroy.clear();

        Bit::Traverse(m_pendingResources.view(), [&](const uint32_t resourceIndex) {
            const Common::ResourceInstance* resource = m_resources[resourceIndex];
            FE_Assert(resource);

            const ResourceSlot& slot = m_resourceSlots[resourceIndex];
            if (IsRetirementComplete(resource, slot, completedGraphicsFence, completedTransferFence))
                resourcesToDestroy.push_back(resourceIndex);
            else
                m_freedResources.set(resourceIndex);
        });

        for (const uint32_t resourceIndex : resourcesToDestroy)
            DestroyResource(resourceIndex);

        m_pendingResources.reset();
        ++m_frameIndex;
    }


    uint32_t ResourcePool::AllocateResourceSlot()
    {
        uint32_t emptySlot = m_emptyResources.find_first();
        if (emptySlot == kInvalidIndex)
        {
            constexpr uint32_t kGrowSize = 1024;

            emptySlot = m_resources.size();
            m_resources.resize(m_resources.size() + kGrowSize);
            m_resourceSlots.resize(m_resourceSlots.size() + kGrowSize);
            m_emptyResources.resize(m_emptyResources.size() + kGrowSize, true);
            m_freedResources.resize(m_freedResources.size() + kGrowSize, false);
            m_pendingResources.resize(m_pendingResources.size() + kGrowSize, false);
        }

        FE_Assert(!m_freedResources.test(emptySlot));
        FE_Assert(!m_pendingResources.test(emptySlot));
        FE_Assert(m_resources[emptySlot] == nullptr);
        m_emptyResources.reset(emptySlot);
        return emptySlot;
    }


    void ResourcePool::FinalizeDecommit(const Common::DetachedResourceInstance& detached)
    {
        Common::ResourceInstance* resourceInstance = detached.m_instance;
        FE_Assert(resourceInstance);

        const uint64_t graphicsQueueFenceValue = m_graphicsQueue->GetTrackedFenceValue();
        const uint64_t transferQueueFenceValue = m_asyncCopyQueue->GetCurrentFence().m_value;
        uint64_t& lastGraphicsFence = resourceInstance->m_lastFenceValues[festd::to_underlying(Core::DeviceQueueType::kGraphics)];
        uint64_t& lastTransferFence = resourceInstance->m_lastFenceValues[festd::to_underlying(Core::DeviceQueueType::kTransfer)];
        lastGraphicsFence = Math::Max(lastGraphicsFence, graphicsQueueFenceValue);
        lastTransferFence = Math::Max(lastTransferFence, transferQueueFenceValue);

        const uint32_t slot = AllocateResourceSlot();
        m_resources[slot] = resourceInstance;
        ResourceSlot& resourceSlot = m_resourceSlots[slot];
        resourceSlot.m_ownerQueue = detached.m_ownerQueue;
        resourceSlot.m_expirationFrame = m_frameIndex + (resourceInstance->m_isTransient ? 1 : 0);
        resourceSlot.m_isReusable = detached.m_isReusable;
        m_pendingResources.set(slot);
    }


    bool ResourcePool::IsRetirementComplete(const Common::ResourceInstance* resourceInstance, const ResourceSlot& slot,
                                            const uint64_t completedGraphicsFence, const uint64_t completedTransferFence) const
    {
        if (m_frameIndex < slot.m_expirationFrame)
            return false;

        const uint64_t graphicsFenceValue =
            resourceInstance->m_lastFenceValues[festd::to_underlying(Core::DeviceQueueType::kGraphics)];
        if (graphicsFenceValue > completedGraphicsFence)
            return false;

        const uint64_t transferFenceValue =
            resourceInstance->m_lastFenceValues[festd::to_underlying(Core::DeviceQueueType::kTransfer)];
        return transferFenceValue <= completedTransferFence;
    }


    void ResourcePool::DestroyResource(const uint32_t resourceIndex)
    {
        Common::ResourceInstance* resourceInstance = m_resources[resourceIndex];
        FE_Assert(resourceInstance);

        switch (resourceInstance->m_type)
        {
        case Core::ResourceType::kBuffer:
            {
                auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(resourceInstance);
                bufferInstance->Invalidate(m_device);
                BufferInstance::Delete(bufferInstance);
                break;
            }
        case Core::ResourceType::kTexture:
            {
                auto* textureInstance = Rtti::AssertCast<TextureInstance*>(resourceInstance);
                textureInstance->Invalidate(m_device);
                TextureInstance::Delete(textureInstance);
                break;
            }
        default:
            FE_DebugBreak();
            break;
        }

        m_resources[resourceIndex] = nullptr;
        m_resourceSlots[resourceIndex] = {};
        m_freedResources.reset(resourceIndex);
        m_pendingResources.reset(resourceIndex);
        m_emptyResources.set(resourceIndex);
    }
} // namespace FE::Graphics::Vulkan
